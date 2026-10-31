/**
 * Bengal Tiger OS - TCP/IP Network Stack Implementation
 *
 * @file net.c
 * @author Bengal Tiger OS (BengalEmpire)
 * @version 1.0.0
 */

#include "net.h"
#include "nic.h"
#include "timer.h"
#include "klog.h"
#include "common.h"

#define ETH_TYPE_IPV4   0x0800
#define ETH_TYPE_ARP    0x0806

#define IP_PROTO_ICMP   1
#define IP_PROTO_UDP    17

#define ARP_OP_REQUEST  1
#define ARP_OP_REPLY    2

#define ICMP_ECHO_REPLY   0
#define ICMP_ECHO_REQUEST 8

#define DHCP_SERVER_PORT  67
#define DHCP_CLIENT_PORT  68
#define DHCP_MAGIC_COOKIE 0x63825363U

#define DHCP_DISCOVER     1
#define DHCP_OFFER        2
#define DHCP_REQUEST      3
#define DHCP_ACK          5

typedef struct {
    uint8_t  dst[6];
    uint8_t  src[6];
    uint16_t ethertype;
} __attribute__((packed)) eth_hdr_t;

typedef struct {
    uint16_t htype;
    uint16_t ptype;
    uint8_t  hlen;
    uint8_t  plen;
    uint16_t oper;
    uint8_t  sha[6];
    uint32_t spa;
    uint8_t  tha[6];
    uint32_t tpa;
} __attribute__((packed)) arp_pkt_t;

typedef struct {
    uint8_t  ver_ihl;
    uint8_t  tos;
    uint16_t total_len;
    uint16_t id;
    uint16_t flags_frag;
    uint8_t  ttl;
    uint8_t   proto;
    uint16_t checksum;
    uint32_t src_ip;
    uint32_t dst_ip;
} __attribute__((packed)) ipv4_hdr_t;

typedef struct {
    uint8_t  type;
    uint8_t  code;
    uint16_t checksum;
    uint16_t id;
    uint16_t seq;
} __attribute__((packed)) icmp_hdr_t;

typedef struct {
    uint16_t src_port;
    uint16_t dst_port;
    uint16_t length;
    uint16_t checksum;
} __attribute__((packed)) udp_hdr_t;

typedef struct {
    uint8_t  op;
    uint8_t  htype;
    uint8_t  hlen;
    uint8_t  hops;
    uint32_t xid;
    uint16_t secs;
    uint16_t flags;
    uint32_t ciaddr;
    uint32_t yiaddr;
    uint32_t siaddr;
    uint32_t giaddr;
    uint8_t  chaddr[16];
    uint8_t  sname[64];
    uint8_t  file[128];
    uint32_t magic;
    uint8_t  options[308];
} __attribute__((packed)) dhcp_pkt_t;

net_config_t net_config;
static arp_entry_t arp_cache[NET_ARP_CACHE_SIZE];
static uint16_t ip_id_counter = 1;

/* Ping reply tracking state */
static volatile int      ping_reply_received = 0;
static volatile uint16_t ping_expected_id    = 0;
static volatile uint16_t ping_expected_seq   = 0;
static volatile uint8_t  ping_reply_ttl      = 0;
static volatile uint32_t ping_reply_tick     = 0;

/* DHCP tracking state */
static volatile int      dhcp_rx_msg_type    = 0;
static volatile uint32_t dhcp_expected_xid   = 0;
static volatile uint32_t dhcp_offered_ip     = 0;
static volatile uint32_t dhcp_offered_mask   = 0;
static volatile uint32_t dhcp_offered_gw     = 0;
static volatile uint32_t dhcp_offered_dns    = 0;
static volatile uint32_t dhcp_offered_srv    = 0;

static const uint8_t bcast_mac[6] = { 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF };

uint16_t net_checksum(const void *data, uint32_t len) {
    const uint8_t *p = (const uint8_t*)data;
    uint32_t sum = 0;

    while (len > 1) {
        uint16_t word = ((uint16_t)p[0] << 8) | (uint16_t)p[1];
        sum += word;
        p   += 2;
        len -= 2;
    }
    if (len == 1) {
        sum += ((uint16_t)p[0] << 8);
    }
    while (sum >> 16) {
        sum = (sum & 0xFFFFU) + (sum >> 16);
    }
    return (uint16_t)(~sum);
}

uint32_t net_make_ip(uint8_t a, uint8_t b, uint8_t c, uint8_t d) {
    return ((uint32_t)a << 24) | ((uint32_t)b << 16) | ((uint32_t)c << 8) | (uint32_t)d;
}

int net_parse_ip(const char *str, uint32_t *out_ip) {
    if (!str || !out_ip) return -1;

    uint32_t parts[4] = {0, 0, 0, 0};
    int idx = 0;
    int has_digit = 0;

    for (int i = 0; ; i++) {
        char c = str[i];
        if (c >= '0' && c <= '9') {
            parts[idx] = parts[idx] * 10 + (uint32_t)(c - '0');
            if (parts[idx] > 255) return -1;
            has_digit = 1;
        } else if (c == '.') {
            if (!has_digit || idx >= 3) return -1;
            idx++;
            has_digit = 0;
        } else if (c == '\0' || c == ' ') {
            if (!has_digit || idx != 3) return -1;
            break;
        } else {
            return -1;
        }
    }

    *out_ip = net_make_ip((uint8_t)parts[0], (uint8_t)parts[1],
                          (uint8_t)parts[2], (uint8_t)parts[3]);
    return 0;
}

void net_format_ip(uint32_t ip, char *buf) {
    if (!buf) return;
    char tmp[8];
    buf[0] = '\0';
    for (int i = 3; i >= 0; i--) {
        uint8_t oct = (uint8_t)((ip >> (i * 8)) & 0xFF);
        k_utoa(oct, tmp, 10);
        strcat(buf, tmp);
        if (i > 0) strcat(buf, ".");
    }
}

void net_init(void) {
    memset(&net_config, 0, sizeof(net_config));
    memset(arp_cache, 0, sizeof(arp_cache));
}

void net_set_static_ip(uint32_t ip, uint32_t netmask, uint32_t gateway, uint32_t dns) {
    net_config.ip          = ip;
    net_config.netmask     = netmask;
    net_config.gateway     = gateway;
    net_config.dns         = dns;
    net_config.dhcp_leased = 0;
    net_config.configured  = (ip != 0) ? 1 : 0;
}

const arp_entry_t* net_get_arp_table(int *max_entries) {
    if (max_entries) *max_entries = NET_ARP_CACHE_SIZE;
    return arp_cache;
}

static void arp_cache_update(uint32_t ip, const uint8_t *mac) {
    if (ip == 0) return;

    int free_slot = -1;
    int oldest_slot = 0;
    uint32_t oldest_tick = 0xFFFFFFFFU;

    for (int i = 0; i < NET_ARP_CACHE_SIZE; i++) {
        if (arp_cache[i].valid && arp_cache[i].ip == ip) {
            memcpy(arp_cache[i].mac, mac, 6);
            arp_cache[i].updated_tick = timer_get_ticks();
            return;
        }
        if (!arp_cache[i].valid && free_slot < 0) {
            free_slot = i;
        }
        if (arp_cache[i].valid && arp_cache[i].updated_tick < oldest_tick) {
            oldest_tick = arp_cache[i].updated_tick;
            oldest_slot = i;
        }
    }

    int slot = (free_slot >= 0) ? free_slot : oldest_slot;
    arp_cache[slot].ip           = ip;
    memcpy(arp_cache[slot].mac, mac, 6);
    arp_cache[slot].updated_tick = timer_get_ticks();
    arp_cache[slot].valid        = 1;
}

static int arp_cache_lookup(uint32_t ip, uint8_t *mac_out) {
    for (int i = 0; i < NET_ARP_CACHE_SIZE; i++) {
        if (arp_cache[i].valid && arp_cache[i].ip == ip) {
            memcpy(mac_out, arp_cache[i].mac, 6);
            return 0;
        }
    }
    return -1;
}

static int send_eth_frame(const uint8_t *dst_mac, uint16_t ethertype,
                          const void *payload, uint16_t payload_len) {
    if (!nic_state.initialized || payload_len > 1500) return -1;

    uint8_t frame[1518];
    eth_hdr_t *eth = (eth_hdr_t*)frame;
    memcpy(eth->dst, dst_mac, 6);
    nic_get_mac(eth->src);
    eth->ethertype = htons(ethertype);
    memcpy(frame + sizeof(eth_hdr_t), payload, payload_len);

    return nic_send_packet(frame, (uint16_t)(sizeof(eth_hdr_t) + payload_len));
}

static void send_arp_packet(uint16_t op, const uint8_t *target_mac, uint32_t target_ip) {
    arp_pkt_t arp;
    arp.htype = htons(1);
    arp.ptype = htons(ETH_TYPE_IPV4);
    arp.hlen  = 6;
    arp.plen  = 4;
    arp.oper  = htons(op);
    nic_get_mac(arp.sha);
    arp.spa   = htonl(net_config.ip);
    if (op == ARP_OP_REQUEST) {
        memset(arp.tha, 0, 6);
    } else {
        memcpy(arp.tha, target_mac, 6);
    }
    arp.tpa   = htonl(target_ip);

    send_eth_frame((op == ARP_OP_REQUEST) ? bcast_mac : target_mac,
                   ETH_TYPE_ARP, &arp, sizeof(arp));
}

int net_arp_resolve(uint32_t target_ip, uint8_t *mac_out, uint32_t timeout_ms) {
    if (!nic_state.initialized) return -1;
    if (target_ip == 0xFFFFFFFFU) {
        memcpy(mac_out, bcast_mac, 6);
        return 0;
    }
    if (arp_cache_lookup(target_ip, mac_out) == 0) {
        return 0;
    }

    uint32_t ticks_per_sec = timer_state.frequency ? timer_state.frequency : 100;
    uint32_t wait_ticks = (timeout_ms * ticks_per_sec) / 1000;
    if (wait_ticks < 5) wait_ticks = 5;

    for (int attempt = 0; attempt < 3; attempt++) {
        send_arp_packet(ARP_OP_REQUEST, bcast_mac, target_ip);

        uint32_t start = timer_get_ticks();
        uint32_t per_try = (wait_ticks / 3) ? (wait_ticks / 3) : 2;
        while ((timer_get_ticks() - start) < per_try) {
            nic_poll();
            if (arp_cache_lookup(target_ip, mac_out) == 0) {
                return 0;
            }
            if (irq_are_enabled()) {
                hlt();
            }
        }
    }
    return -1;
}

static uint32_t route_next_hop(uint32_t dst_ip) {
    if (dst_ip == 0xFFFFFFFFU) return 0xFFFFFFFFU;
    if (net_config.netmask != 0 && net_config.gateway != 0) {
        if ((dst_ip & net_config.netmask) != (net_config.ip & net_config.netmask)) {
            return net_config.gateway;
        }
    }
    return dst_ip;
}

static int send_ipv4_packet(uint32_t src_ip, uint32_t dst_ip, const uint8_t *dst_mac_override,
                            uint8_t proto, const void *payload, uint16_t payload_len) {
    if (sizeof(ipv4_hdr_t) + payload_len > 1500) return -1;

    uint8_t dst_mac[6];
    if (dst_mac_override) {
        memcpy(dst_mac, dst_mac_override, 6);
    } else {
        uint32_t next_hop = route_next_hop(dst_ip);
        if (net_arp_resolve(next_hop, dst_mac, 800) != 0) {
            return -1;
        }
    }

    uint8_t pkt[1500];
    ipv4_hdr_t *ip = (ipv4_hdr_t*)pkt;
    ip->ver_ihl    = 0x45;
    ip->tos        = 0;
    ip->total_len  = htons((uint16_t)(sizeof(ipv4_hdr_t) + payload_len));
    ip->id         = htons(ip_id_counter++);
    ip->flags_frag = 0;
    ip->ttl        = 64;
    ip->proto      = proto;
    ip->checksum   = 0;
    ip->src_ip     = htonl(src_ip);
    ip->dst_ip     = htonl(dst_ip);
    ip->checksum   = htons(net_checksum(ip, sizeof(ipv4_hdr_t)));

    memcpy(pkt + sizeof(ipv4_hdr_t), payload, payload_len);
    return send_eth_frame(dst_mac, ETH_TYPE_IPV4, pkt,
                          (uint16_t)(sizeof(ipv4_hdr_t) + payload_len));
}

int net_send_udp(uint32_t dst_ip, uint16_t src_port, uint16_t dst_port,
                 const void *payload, uint16_t payload_len) {
    if (sizeof(udp_hdr_t) + payload_len > 1472) return -1;

    uint8_t buf[1480];
    udp_hdr_t *udp = (udp_hdr_t*)buf;
    udp->src_port  = htons(src_port);
    udp->dst_port  = htons(dst_port);
    udp->length    = htons((uint16_t)(sizeof(udp_hdr_t) + payload_len));
    udp->checksum  = 0; /* Optional in IPv4 UDP (RFC 768) */

    if (payload_len > 0 && payload) {
        memcpy(buf + sizeof(udp_hdr_t), payload, payload_len);
    }

    const uint8_t *mac_ovr = (dst_ip == 0xFFFFFFFFU) ? bcast_mac : NULL;
    return send_ipv4_packet(net_config.ip, dst_ip, mac_ovr, IP_PROTO_UDP,
                            buf, (uint16_t)(sizeof(udp_hdr_t) + payload_len));
}

int net_ping(uint32_t dst_ip, uint16_t seq, uint32_t timeout_ms,
             uint32_t *out_rtt_ms, uint8_t *out_ttl) {
    if (!nic_state.initialized || !net_config.configured) return -1;

    /* Resolve ARP before starting RTT stopwatch so RTT reflects actual ICMP echo */
    uint8_t dst_mac[6];
    uint32_t next_hop = route_next_hop(dst_ip);
    if (net_arp_resolve(next_hop, dst_mac, timeout_ms) != 0) {
        return -2; /* Host/gateway unreachable (ARP timeout) */
    }

    uint8_t pkt[sizeof(icmp_hdr_t) + 32];
    icmp_hdr_t *icmp = (icmp_hdr_t*)pkt;
    uint16_t pid = 0xB705;

    icmp->type     = ICMP_ECHO_REQUEST;
    icmp->code     = 0;
    icmp->checksum = 0;
    icmp->id       = htons(pid);
    icmp->seq      = htons(seq);

    for (int i = 0; i < 32; i++) {
        pkt[sizeof(icmp_hdr_t) + i] = (uint8_t)('A' + (i % 26));
    }
    icmp->checksum = htons(net_checksum(pkt, sizeof(pkt)));

    ping_expected_id    = pid;
    ping_expected_seq   = seq;
    ping_reply_received = 0;

    uint32_t start_tick = timer_get_ticks();
    if (send_ipv4_packet(net_config.ip, dst_ip, dst_mac, IP_PROTO_ICMP,
                         pkt, sizeof(pkt)) != 0) {
        return -1;
    }

    uint32_t freq = timer_state.frequency ? timer_state.frequency : 100;
    uint32_t max_ticks = (timeout_ms * freq) / 1000;
    if (max_ticks < 2) max_ticks = 2;

    while ((timer_get_ticks() - start_tick) <= max_ticks) {
        nic_poll();
        if (ping_reply_received) {
            uint32_t elapsed_ticks = ping_reply_tick - start_tick;
            if (out_rtt_ms) *out_rtt_ms = (elapsed_ticks * 1000) / freq;
            if (out_ttl)    *out_ttl    = ping_reply_ttl;
            return 0;
        }
        if (irq_are_enabled()) {
            hlt();
        }
    }

    return -1;
}

static int send_dhcp_message(uint8_t msg_type, uint32_t xid,
                             uint32_t req_ip, uint32_t srv_ip) {
    dhcp_pkt_t dhcp;
    memset(&dhcp, 0, sizeof(dhcp));

    dhcp.op    = 1; /* BOOTREQUEST */
    dhcp.htype = 1; /* Ethernet */
    dhcp.hlen  = 6;
    dhcp.hops  = 0;
    dhcp.xid   = htonl(xid);
    dhcp.secs  = 0;
    dhcp.flags = htons(0x8000); /* Broadcast flag */
    nic_get_mac(dhcp.chaddr);
    dhcp.magic = htonl(DHCP_MAGIC_COOKIE);

    int o = 0;
    /* Option 53: DHCP Message Type */
    dhcp.options[o++] = 53;
    dhcp.options[o++] = 1;
    dhcp.options[o++] = msg_type;

    if (msg_type == DHCP_REQUEST && req_ip != 0) {
        /* Option 50: Requested IP Address */
        uint32_t nip = htonl(req_ip);
        dhcp.options[o++] = 50;
        dhcp.options[o++] = 4;
        memcpy(&dhcp.options[o], &nip, 4);
        o += 4;
    }

    if (msg_type == DHCP_REQUEST && srv_ip != 0) {
        /* Option 54: DHCP Server Identifier */
        uint32_t nsrv = htonl(srv_ip);
        dhcp.options[o++] = 54;
        dhcp.options[o++] = 4;
        memcpy(&dhcp.options[o], &nsrv, 4);
        o += 4;
    }

    /* Option 55: Parameter Request List (Mask, Router, DNS) */
    dhcp.options[o++] = 55;
    dhcp.options[o++] = 3;
    dhcp.options[o++] = 1;
    dhcp.options[o++] = 3;
    dhcp.options[o++] = 6;

    /* End Option */
    dhcp.options[o++] = 255;

    uint8_t udp_buf[sizeof(udp_hdr_t) + sizeof(dhcp_pkt_t)];
    udp_hdr_t *udp = (udp_hdr_t*)udp_buf;
    udp->src_port  = htons(DHCP_CLIENT_PORT);
    udp->dst_port  = htons(DHCP_SERVER_PORT);
    udp->length    = htons((uint16_t)sizeof(udp_buf));
    udp->checksum  = 0;
    memcpy(udp_buf + sizeof(udp_hdr_t), &dhcp, sizeof(dhcp_pkt_t));

    return send_ipv4_packet(0, 0xFFFFFFFFU, bcast_mac, IP_PROTO_UDP,
                            udp_buf, sizeof(udp_buf));
}

int net_dhcp_discover(uint32_t timeout_ms) {
    if (!nic_state.initialized) return -1;

    uint32_t xid = 0xBE6A1000U ^ (timer_get_ticks() + 1U);
    dhcp_expected_xid = xid;
    dhcp_rx_msg_type  = 0;
    dhcp_offered_ip   = 0;
    dhcp_offered_mask = net_make_ip(255, 255, 255, 0);
    dhcp_offered_gw   = 0;
    dhcp_offered_dns  = 0;
    dhcp_offered_srv  = 0;

    if (send_dhcp_message(DHCP_DISCOVER, xid, 0, 0) != 0) return -1;

    uint32_t freq = timer_state.frequency ? timer_state.frequency : 100;
    uint32_t half_ticks = ((timeout_ms / 2) * freq) / 1000;
    if (half_ticks < 5) half_ticks = 5;

    /* Wait for DHCPOFFER */
    uint32_t start = timer_get_ticks();
    while ((timer_get_ticks() - start) < half_ticks) {
        nic_poll();
        if (dhcp_rx_msg_type == DHCP_OFFER && dhcp_offered_ip != 0) {
            break;
        }
        if (irq_are_enabled()) hlt();
    }

    if (dhcp_rx_msg_type != DHCP_OFFER || dhcp_offered_ip == 0) {
        return -1;
    }

    uint32_t req_ip = dhcp_offered_ip;
    uint32_t srv_ip = dhcp_offered_srv;
    dhcp_rx_msg_type = 0;

    if (send_dhcp_message(DHCP_REQUEST, xid, req_ip, srv_ip) != 0) return -1;

    /* Wait for DHCPACK */
    start = timer_get_ticks();
    while ((timer_get_ticks() - start) < half_ticks) {
        nic_poll();
        if (dhcp_rx_msg_type == DHCP_ACK) {
            break;
        }
        if (irq_are_enabled()) hlt();
    }

    if (dhcp_rx_msg_type != DHCP_ACK && req_ip == 0) {
        return -1;
    }

    net_config.ip          = dhcp_offered_ip ? dhcp_offered_ip : req_ip;
    net_config.netmask     = dhcp_offered_mask ? dhcp_offered_mask : net_make_ip(255, 255, 255, 0);
    net_config.gateway     = dhcp_offered_gw;
    net_config.dns         = dhcp_offered_dns;
    net_config.dhcp_server = dhcp_offered_srv;
    net_config.dhcp_leased = 1;
    net_config.configured  = 1;
    return 0;
}

void net_autoconfig(void) {
    if (!nic_state.initialized) return;

    if (net_dhcp_discover(900) == 0) {
        char ip_str[20];
        net_format_ip(net_config.ip, ip_str);
        char msg[64] = "DHCP lease acquired: ";
        strcat(msg, ip_str);
        klog_writeln("net", msg);
    } else {
        /* Fallback static configuration for isolated links */
        net_set_static_ip(net_make_ip(10, 0, 2, 15),
                          net_make_ip(255, 255, 255, 0),
                          net_make_ip(10, 0, 2, 2),
                          net_make_ip(10, 0, 2, 3));
        klog_writeln("net", "No DHCP server; configured fallback 10.0.2.15/24");
    }
}

/* =========================================================================
 * Incoming Packet Dispatcher
 * ========================================================================= */
static void handle_arp(const uint8_t *payload, uint16_t len) {
    if (len < sizeof(arp_pkt_t)) return;
    const arp_pkt_t *arp = (const arp_pkt_t*)payload;

    if (ntohs(arp->htype) != 1 || ntohs(arp->ptype) != ETH_TYPE_IPV4) return;
    net_config.rx_arp++;

    uint32_t sender_ip = ntohl(arp->spa);
    uint32_t target_ip = ntohl(arp->tpa);
    uint16_t oper      = ntohs(arp->oper);

    arp_cache_update(sender_ip, arp->sha);

    if (oper == ARP_OP_REQUEST && net_config.configured && target_ip == net_config.ip) {
        send_arp_packet(ARP_OP_REPLY, arp->sha, sender_ip);
    }
}

static void handle_icmp(uint32_t src_ip, uint8_t ttl, const uint8_t *payload, uint16_t len) {
    if (len < sizeof(icmp_hdr_t)) return;
    const icmp_hdr_t *icmp = (const icmp_hdr_t*)payload;
    net_config.rx_icmp++;

    if (icmp->type == ICMP_ECHO_REQUEST && net_config.configured) {
        uint8_t reply[1480];
        if (len > sizeof(reply)) return;
        memcpy(reply, payload, len);
        icmp_hdr_t *rh = (icmp_hdr_t*)reply;
        rh->type     = ICMP_ECHO_REPLY;
        rh->code     = 0;
        rh->checksum = 0;
        rh->checksum = htons(net_checksum(reply, len));
        send_ipv4_packet(net_config.ip, src_ip, NULL, IP_PROTO_ICMP, reply, len);
    } else if (icmp->type == ICMP_ECHO_REPLY) {
        if (ntohs(icmp->id) == ping_expected_id &&
            ntohs(icmp->seq) == ping_expected_seq) {
            ping_reply_ttl      = ttl;
            ping_reply_tick     = timer_get_ticks();
            ping_reply_received = 1;
        }
    }
}

static void handle_dhcp(const uint8_t *payload, uint16_t len) {
    /* Minimum BOOTP + magic cookie = 240 bytes */
    if (len < 240) return;
    const dhcp_pkt_t *dhcp = (const dhcp_pkt_t*)payload;

    if (dhcp->op != 2) return; /* BOOTREPLY */
    if (ntohl(dhcp->xid) != dhcp_expected_xid) return;
    if (ntohl(dhcp->magic) != DHCP_MAGIC_COOKIE) return;

    uint8_t our_mac[6];
    nic_get_mac(our_mac);
    if (memcmp(dhcp->chaddr, our_mac, 6) != 0) return;

    uint32_t yiaddr = ntohl(dhcp->yiaddr);
    uint16_t opt_len = len - 240;
    const uint8_t *opts = payload + 240;

    uint8_t msg_type = 0;
    uint32_t mask = 0, gw = 0, dns = 0, srv = 0;

    uint16_t i = 0;
    while (i < opt_len) {
        uint8_t code = opts[i++];
        if (code == 0) continue;
        if (code == 255) break;
        if (i >= opt_len) break;
        uint8_t olen = opts[i++];
        if (i + olen > opt_len) break;

        if (code == 53 && olen >= 1) {
            msg_type = opts[i];
        } else if (code == 1 && olen >= 4) {
            uint32_t v; memcpy(&v, &opts[i], 4); mask = ntohl(v);
        } else if (code == 3 && olen >= 4) {
            uint32_t v; memcpy(&v, &opts[i], 4); gw = ntohl(v);
        } else if (code == 6 && olen >= 4) {
            uint32_t v; memcpy(&v, &opts[i], 4); dns = ntohl(v);
        } else if (code == 54 && olen >= 4) {
            uint32_t v; memcpy(&v, &opts[i], 4); srv = ntohl(v);
        }
        i += olen;
    }

    if (msg_type == DHCP_OFFER || msg_type == DHCP_ACK) {
        if (yiaddr) dhcp_offered_ip   = yiaddr;
        if (mask)   dhcp_offered_mask = mask;
        if (gw)     dhcp_offered_gw   = gw;
        if (dns)    dhcp_offered_dns  = dns;
        if (srv)    dhcp_offered_srv  = srv;
        dhcp_rx_msg_type = msg_type;
    }
}

static void handle_udp(const uint8_t *payload, uint16_t len) {
    if (len < sizeof(udp_hdr_t)) return;
    const udp_hdr_t *udp = (const udp_hdr_t*)payload;
    uint16_t ulen = ntohs(udp->length);
    if (ulen < sizeof(udp_hdr_t) || ulen > len) return;

    net_config.rx_udp++;

    uint16_t dst_port = ntohs(udp->dst_port);
    if (dst_port == DHCP_CLIENT_PORT) {
        handle_dhcp(payload + sizeof(udp_hdr_t), (uint16_t)(ulen - sizeof(udp_hdr_t)));
    }
}

static void handle_ipv4(const uint8_t *eth_src, const uint8_t *payload, uint16_t len) {
    if (len < sizeof(ipv4_hdr_t)) return;
    const ipv4_hdr_t *ip = (const ipv4_hdr_t*)payload;

    if ((ip->ver_ihl >> 4) != 4) return;
    uint16_t ihl = (uint16_t)(ip->ver_ihl & 0x0F) * 4;
    if (ihl < 20 || ihl > len) return;

    uint16_t total_len = ntohs(ip->total_len);
    if (total_len < ihl || total_len > len) return;

    if (net_checksum(ip, ihl) != 0) return;

    net_config.rx_ipv4++;

    uint32_t src_ip = ntohl(ip->src_ip);
    uint32_t dst_ip = ntohl(ip->dst_ip);

    /* Opportunistically cache sender MAC if on local subnet */
    if (src_ip != 0 && net_config.netmask != 0 &&
        (src_ip & net_config.netmask) == (net_config.ip & net_config.netmask)) {
        arp_cache_update(src_ip, eth_src);
    }

    /* Accept broadcast, DHCP unconfigured frames, or packets addressed to us */
    if (dst_ip != 0xFFFFFFFFU && net_config.configured && dst_ip != net_config.ip) {
        return;
    }

    const uint8_t *l4 = payload + ihl;
    uint16_t l4_len   = (uint16_t)(total_len - ihl);

    if (ip->proto == IP_PROTO_ICMP) {
        handle_icmp(src_ip, ip->ttl, l4, l4_len);
    } else if (ip->proto == IP_PROTO_UDP) {
        handle_udp(l4, l4_len);
    }
}

void net_rx_packet(const uint8_t *data, uint16_t len) {
    if (!data || len < sizeof(eth_hdr_t)) return;
    net_config.rx_eth++;

    const eth_hdr_t *eth = (const eth_hdr_t*)data;
    uint16_t ethertype   = ntohs(eth->ethertype);
    const uint8_t *pl    = data + sizeof(eth_hdr_t);
    uint16_t pl_len      = (uint16_t)(len - sizeof(eth_hdr_t));

    if (ethertype == ETH_TYPE_ARP) {
        handle_arp(pl, pl_len);
    } else if (ethertype == ETH_TYPE_IPV4) {
        handle_ipv4(eth->src, pl, pl_len);
    }
}
