/**
 * Bengal Tiger OS - TCP/IP Network Stack (Ethernet, ARP, IPv4, ICMP, UDP, DHCP)
 *
 * Implements real packet framing, ARP resolution & caching, IPv4 subnet
 * routing, ICMP Echo Request/Reply (`ping`), UDP datagrams, and a full
 * 4-way DORA DHCPv4 client.
 *
 * All IPv4 `uint32_t` parameters in public functions use host byte order
 * (e.g. `10.0.2.15` == `0x0A00020F`), converted via `htonl`/`ntohl` on wire.
 *
 * @file net.h
 * @author Bengal Tiger OS (BengalEmpire)
 * @version 1.0.0
 */

#ifndef NET_H
#define NET_H

#include "common.h"

#define NET_ARP_CACHE_SIZE  16

typedef struct {
    uint32_t ip;
    uint8_t  mac[6];
    uint32_t updated_tick;
    int      valid;
} arp_entry_t;

typedef struct {
    int      configured;
    int      dhcp_leased;
    uint32_t ip;
    uint32_t netmask;
    uint32_t gateway;
    uint32_t dns;
    uint32_t dhcp_server;
    uint32_t rx_eth;
    uint32_t rx_arp;
    uint32_t rx_ipv4;
    uint32_t rx_icmp;
    uint32_t rx_udp;
} net_config_t;

extern net_config_t net_config;

void     net_init(void);
void     net_autoconfig(void);
void     net_rx_packet(const uint8_t *data, uint16_t len);

uint16_t net_checksum(const void *data, uint32_t len);
uint32_t net_make_ip(uint8_t a, uint8_t b, uint8_t c, uint8_t d);
int      net_parse_ip(const char *str, uint32_t *out_ip);
void     net_format_ip(uint32_t ip, char *buf);

void     net_set_static_ip(uint32_t ip, uint32_t netmask, uint32_t gateway, uint32_t dns);
int      net_dhcp_discover(uint32_t timeout_ms);
int      net_arp_resolve(uint32_t target_ip, uint8_t *mac_out, uint32_t timeout_ms);
int      net_ping(uint32_t dst_ip, uint16_t seq, uint32_t timeout_ms,
                  uint32_t *out_rtt_ms, uint8_t *out_ttl);
int      net_send_udp(uint32_t dst_ip, uint16_t src_port, uint16_t dst_port,
                      const void *payload, uint16_t payload_len);
const arp_entry_t* net_get_arp_table(int *max_entries);

#endif /* NET_H */
