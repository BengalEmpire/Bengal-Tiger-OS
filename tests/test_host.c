/**
 * Bengal Tiger OS - Comprehensive Host Unit & Subsystem Test Suite
 *
 * Compiles with AddressSanitizer + UndefinedBehaviorSanitizer on the host
 * (`make test-unit`) and directly tests kernel subsystems:
 *   1. String/integer formatting & endianness primitives (`common.c`)
 *   2. Circular diagnostic kernel log (`klog.c`)
 *   3. Expandable free-list kernel heap + aligned allocations (`heap.c`)
 *   4. FAT12, FAT16, MBR partition mounting, multi-cluster R/W, overwrite
 *      cluster reclamation, append, copy, rename & delete (`fat.c`)
 *   5. RFC 1071 checksums, ARP request/reply, IPv4, ICMP Echo (`ping`),
 *      and full 4-way DHCPv4 DORA client (`net.c`)
 */

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

#include "common.h"
#include "klog.h"
#include "heap.h"
#include "disk.h"
#include "fat.h"
#include "nic.h"
#include "net.h"
#include "timer.h"

/* =========================================================================
 * Hardware Simulation Harness for Host Testing
 * ========================================================================= */
timer_state_t timer_state = { 100, 1, 100 };
static uint32_t sim_ticks = 100;

uint32_t timer_get_ticks(void) { return ++sim_ticks; }
uint32_t timer_get_ms(void)    { return sim_ticks * 10; }

void serial_write_str(const char *str) { (void)str; }
void shell_print(const char *str)      { (void)str; }
void shell_print_color(const char *str, uint8_t c) { (void)str; (void)c; }
void shell_print_int(int num)          { (void)num; }

/* Simulated Block Storage (up to 32,768 sectors = 16 MB) */
#define SIM_MAX_SECTORS 32768
static uint8_t  sim_ata_disk[SIM_MAX_SECTORS][512];
static uint8_t  sim_ram_disk[RAMDISK_SECTORS][512];
static uint32_t sim_ata_sectors = 32768;
static int      sim_has_ata     = 0;
static int      sim_use_ramdisk = 1;

int disk_has_hardware_hdd(void) { return sim_has_ata; }
int disk_is_ramdisk(void)       { return sim_use_ramdisk; }

int disk_enable_ramdisk(int enable) {
    if (enable) {
        sim_use_ramdisk = 1;
        return 0;
    }
    if (!sim_has_ata) return -1;
    sim_use_ramdisk = 0;
    return 0;
}

uint32_t disk_get_total_sectors(void) {
    return sim_use_ramdisk ? RAMDISK_SECTORS : sim_ata_sectors;
}

int ata_read_sector(uint32_t lba, uint8_t *buf) {
    if (sim_use_ramdisk) {
        if (lba >= RAMDISK_SECTORS) return -1;
        memcpy(buf, sim_ram_disk[lba], 512);
        return 0;
    }
    if (lba >= sim_ata_sectors) return -1;
    memcpy(buf, sim_ata_disk[lba], 512);
    return 0;
}

int ata_write_sector(uint32_t lba, const uint8_t *buf) {
    if (sim_use_ramdisk) {
        if (lba >= RAMDISK_SECTORS) return -1;
        memcpy(sim_ram_disk[lba], buf, 512);
        return 0;
    }
    if (lba >= sim_ata_sectors) return -1;
    memcpy(sim_ata_disk[lba], buf, 512);
    return 0;
}

/* Simulated NIC + Virtual Gateway/DHCP Server (`10.0.2.2`, MAC `52:55:0A:00:02:02`) */
nic_state_t nic_state;
static const uint8_t gw_mac[6] = { 0x52, 0x55, 0x0A, 0x00, 0x02, 0x02 };
static uint8_t last_tx_frame[1600];
static uint16_t last_tx_len = 0;
static uint8_t pending_rx_frame[1600];
static uint16_t pending_rx_len = 0;

void nic_get_mac(uint8_t *mac_out) {
    memcpy(mac_out, nic_state.mac, 6);
}

static void build_virtual_dhcp_reply(const uint8_t *req_dhcp, uint16_t req_len) {
    if (req_len < 244) return;
    uint8_t req_type = 0;
    for (uint16_t i = 240; i + 2 < req_len; ) {
        uint8_t code = req_dhcp[i++];
        if (code == 0) continue;
        if (code == 255) break;
        uint8_t olen = req_dhcp[i++];
        if (code == 53 && olen >= 1) req_type = req_dhcp[i];
        i += olen;
    }
    if (req_type != 1 && req_type != 3) return;
    uint8_t reply_type = (req_type == 1) ? 2 : 5; /* OFFER or ACK */

    uint8_t frame[600];
    memset(frame, 0, sizeof(frame));
    /* Eth header */
    memcpy(frame + 0, nic_state.mac, 6);
    memcpy(frame + 6, gw_mac, 6);
    frame[12] = 0x08; frame[13] = 0x00;

    /* IPv4 header (20 bytes) */
    uint8_t *ip = frame + 14;
    uint16_t dhcp_len = 240 + 32;
    uint16_t udp_len  = 8 + dhcp_len;
    uint16_t ip_len   = 20 + udp_len;
    ip[0] = 0x45;
    ip[2] = (uint8_t)(ip_len >> 8); ip[3] = (uint8_t)(ip_len & 0xFF);
    ip[8] = 64;
    ip[9] = 17; /* UDP */
    ip[12] = 10; ip[13] = 0; ip[14] = 2; ip[15] = 2;
    ip[16] = 255; ip[17] = 255; ip[18] = 255; ip[19] = 255;
    uint16_t csum = htons(net_checksum(ip, 20));
    memcpy(ip + 10, &csum, 2);

    /* UDP header */
    uint8_t *udp = ip + 20;
    udp[0] = 0; udp[1] = 67;
    udp[2] = 0; udp[3] = 68;
    udp[4] = (uint8_t)(udp_len >> 8); udp[5] = (uint8_t)(udp_len & 0xFF);

    /* BOOTP/DHCP payload */
    uint8_t *d = udp + 8;
    d[0] = 2; d[1] = 1; d[2] = 6;
    memcpy(d + 4, req_dhcp + 4, 4); /* xid */
    d[16] = 10; d[17] = 0; d[18] = 2; d[19] = 15; /* yiaddr = 10.0.2.15 */
    memcpy(d + 28, nic_state.mac, 6);
    d[236] = 0x63; d[237] = 0x82; d[238] = 0x53; d[239] = 0x63;

    int o = 240;
    d[o++] = 53; d[o++] = 1; d[o++] = reply_type;
    d[o++] = 1;  d[o++] = 4; d[o++] = 255; d[o++] = 255; d[o++] = 255; d[o++] = 0;
    d[o++] = 3;  d[o++] = 4; d[o++] = 10;  d[o++] = 0;   d[o++] = 2;   d[o++] = 2;
    d[o++] = 6;  d[o++] = 4; d[o++] = 10;  d[o++] = 0;   d[o++] = 2;   d[o++] = 3;
    d[o++] = 54; d[o++] = 4; d[o++] = 10;  d[o++] = 0;   d[o++] = 2;   d[o++] = 2;
    d[o++] = 255;

    pending_rx_len = (uint16_t)(14 + ip_len);
    memcpy(pending_rx_frame, frame, pending_rx_len);
}

int nic_send_packet(const void *data, uint16_t length) {
    assert(length <= sizeof(last_tx_frame));
    memcpy(last_tx_frame, data, length);
    last_tx_len = length;
    nic_state.tx_packets++;

    const uint8_t *f = (const uint8_t*)data;
    uint16_t etype = ((uint16_t)f[12] << 8) | f[13];

    /* Auto-respond as virtual router 10.0.2.2 for ARP, Ping, and DHCP */
    if (etype == 0x0806 && length >= 42) {
        uint16_t oper = ((uint16_t)f[20] << 8) | f[21];
        if (oper == 1 && f[38] == 10 && f[39] == 0 && f[40] == 2 && f[41] == 2) {
            uint8_t rep[42];
            memcpy(rep, f + 6, 6);
            memcpy(rep + 6, gw_mac, 6);
            rep[12] = 0x08; rep[13] = 0x06;
            rep[14] = 0; rep[15] = 1; rep[16] = 0x08; rep[17] = 0x00;
            rep[18] = 6; rep[19] = 4; rep[20] = 0; rep[21] = 2; /* ARP REPLY */
            memcpy(rep + 22, gw_mac, 6);
            rep[28] = 10; rep[29] = 0; rep[30] = 2; rep[31] = 2;
            memcpy(rep + 32, f + 22, 10);
            memcpy(pending_rx_frame, rep, 42);
            pending_rx_len = 42;
        }
    } else if (etype == 0x0800 && length >= 34) {
        uint8_t proto = f[23];
        if (proto == 17 && length >= 42 + 240) {
            uint16_t dport = ((uint16_t)f[36] << 8) | f[37];
            if (dport == 67) {
                build_virtual_dhcp_reply(f + 42, (uint16_t)(length - 42));
            }
        } else if (proto == 1 && f[34] == 8) {
            /* Auto-reply ICMP Echo Reply from 10.0.2.2 */
            uint8_t rep[1500];
            memcpy(rep, f, length);
            memcpy(rep + 0, f + 6, 6);
            memcpy(rep + 6, gw_mac, 6);
            memcpy(rep + 26, f + 30, 4);
            memcpy(rep + 30, f + 26, 4);
            rep[22] = 64;
            rep[24] = 0; rep[25] = 0;
            uint16_t ip_cs = htons(net_checksum(rep + 14, 20));
            memcpy(rep + 24, &ip_cs, 2);

            rep[34] = 0; /* Echo Reply */
            rep[36] = 0; rep[37] = 0;
            uint16_t icmp_cs = htons(net_checksum(rep + 34, (uint32_t)(length - 34)));
            memcpy(rep + 36, &icmp_cs, 2);

            memcpy(pending_rx_frame, rep, length);
            pending_rx_len = length;
        }
    }
    return 0;
}

void nic_poll(void) {
    if (pending_rx_len > 0) {
        uint8_t tmp[1600];
        uint16_t l = pending_rx_len;
        pending_rx_len = 0;
        memcpy(tmp, pending_rx_frame, l);
        net_rx_packet(tmp, l);
    }
}

/* =========================================================================
 * Unit Tests
 * ========================================================================= */
static void test_common_and_klog(void) {
    char buf[36];
    assert(k_atoi("  -4291") == -4291);
    assert(k_atoi("+1024") == 1024);

    k_itoa(-2147483647 - 1, buf, 10);
    assert(strcmp(buf, "-2147483648") == 0);

    k_utoa(0xDEADBEEFU, buf, 16);
    assert(strcmp(buf, "DEADBEEF") == 0);

    assert(htons(0x1234) == 0x3412);
    assert(htonl(0x0A00020FU) == 0x0F02000AU);

    klog_init();
    for (int i = 0; i < 300; i++) {
        klog_writeln("test", "Circular diagnostic ring buffer stress line 0123456789");
    }
    uint32_t len = 0;
    const char *log = klog_get_buffer(&len);
    assert(len > 0 && len < KLOG_BUFFER_SIZE);
    assert(log[len] == '\0');
    assert(strstr(log, "Circular diagnostic ring buffer") != NULL);
    printf("[PASS] common & klog circular ring buffer\n");
}

static void test_heap_allocator(void) {
    size_t max_arena = 4 * 1024 * 1024;
    void *arena = calloc(1, max_arena);
    assert(arena != NULL);

    /* Start with 512KB initial size, max 4MB to test dynamic expansion */
    heap_init_at(arena, 512 * 1024, (uint32_t)max_arena);
    assert(heap_check() == 1);

    void *p1 = kmalloc(1024);
    void *p2 = kzalloc(4096);
    void *pa = kmalloc_aligned(512, 4096);
    assert(p1 && p2 && pa);
    assert(((uintptr_t)pa & 4095) == 0);

    for (int i = 0; i < 4096; i++) assert(((uint8_t*)p2)[i] == 0);

    /* Trigger dynamic heap expansion (> 512KB) */
    void *big = kmalloc(900 * 1024);
    assert(big != NULL);
    memset(big, 0x5A, 900 * 1024);

    p1 = krealloc(p1, 8192);
    assert(p1 != NULL);

    kfree(pa);
    kfree(p2);
    kfree(big);
    kfree(p1);

    assert(heap_check() == 1);
    heap_stats_t st;
    heap_get_stats(&st);
    assert(st.used_size == 0);
    assert(st.num_allocations == 0);
    assert(st.num_blocks == 1); /* Coalesced back into 1 single contiguous block! */

    /* Leave heap initialized for subsequent FAT tests */
    printf("[PASS] expandable kernel heap, aligned free & full coalescing\n");
    /* Note: arena intentionally kept alive for FAT tests below */
}

static void test_fat12_fat16_and_mbr(void) {
    memset(sim_ram_disk, 0, sizeof(sim_ram_disk));
    memset(sim_ata_disk, 0, sizeof(sim_ata_disk));
    sim_has_ata = 0;

    /* 1. Live RAMDisk FAT12 auto-init */
    fat_init();
    assert(fat_is_mounted() == 1);

    fat_fs_info_t fi;
    fat_get_fs_info(&fi);
    assert(fi.fat_type == 12);
    assert(fi.is_ramdisk == 1);
    assert(fat_file_exists("HELLO.TXT") == 1);
    assert(fat_file_exists("README.TXT") == 1);
    assert(fat_file_exists("DEMO.SH") == 1);

    uint32_t baseline_used = fi.used_clusters;

    /* 2. Multi-cluster write (6,000 bytes = 12 sectors/clusters), read, overwrite & leak check */
    uint8_t *pattern = (uint8_t*)malloc(6000);
    uint8_t *readback = (uint8_t*)malloc(7000);
    for (int i = 0; i < 6000; i++) pattern[i] = (uint8_t)((i * 37 + 13) & 0xFF);

    assert(fat_write_file("bigdata.bin", pattern, 6000) == 0);
    uint32_t out_sz = 0;
    assert(fat_read_file("BIGDATA.BIN", readback, 7000, &out_sz) == 0);
    assert(out_sz == 6000);
    assert(memcmp(pattern, readback, 6000) == 0);

    fat_get_fs_info(&fi);
    assert(fi.used_clusters == baseline_used + 12);

    /* Overwrite with tiny 50-byte payload -> 11 clusters MUST be freed! */
    assert(fat_write_file("bigdata.bin", "Short payload", 13) == 0);
    fat_get_fs_info(&fi);
    assert(fi.used_clusters == baseline_used + 1);

    /* Append, Copy, Rename, Delete */
    assert(fat_append_file("bigdata.bin", " + appended", 11) == 0);
    assert(fat_copy_file("bigdata.bin", "copy.bin") == 0);
    assert(fat_rename_file("copy.bin", "moved.bin") == 0);
    assert(fat_file_exists("COPY.BIN") == 0);
    assert(fat_read_file("MOVED.BIN", readback, 100, &out_sz) == 0);
    assert(out_sz == 24);
    assert(memcmp(readback, "Short payload + appended", 24) == 0);

    assert(fat_delete_file("bigdata.bin") == 0);
    assert(fat_delete_file("moved.bin") == 0);
    fat_get_fs_info(&fi);
    assert(fi.used_clusters == baseline_used);

    /* 3. Format 16MB Hardware ATA Disk as FAT16 */
    sim_has_ata = 1;
    sim_ata_sectors = 32768; /* 16 MB -> > 4085 clusters -> FAT16 */
    assert(fat_format("BENGAL16", 1) == 0);
    fat_get_fs_info(&fi);
    assert(fi.fat_type == 16);
    assert(fi.is_ramdisk == 0);
    assert(strcmp(fi.label, "BENGAL16") == 0);

    assert(fat_write_file("persist.txt", "FAT16_OK", 8) == 0);

    /* 4. Relocate volume to MBR partition 1 @ LBA 2048 and verify MBR mount! */
    uint32_t part_lba = 2048;
    uint32_t part_secs = 28000;
    sim_ata_sectors = 28000;
    assert(fat_format("MBRPART", 1) == 0);
    assert(fat_write_file("mbrtest.txt", "MBR_MOUNT_SUCCESS", 17) == 0);

    sim_ata_sectors = 32768;
    memmove(sim_ata_disk[part_lba], sim_ata_disk[0], part_secs * 512);
    memset(sim_ata_disk[0], 0, 512);
    /* Write MBR partition entry 0 at 0x1BE */
    uint8_t *p0 = &sim_ata_disk[0][0x1BE];
    p0[0]  = 0x80;
    p0[4]  = 0x06; /* FAT16B */
    p0[8]  = (uint8_t)(part_lba & 0xFF);
    p0[9]  = (uint8_t)((part_lba >> 8) & 0xFF);
    p0[12] = (uint8_t)(part_secs & 0xFF);
    p0[13] = (uint8_t)((part_secs >> 8) & 0xFF);
    sim_ata_disk[0][510] = 0x55;
    sim_ata_disk[0][511] = 0xAA;

    fat_init();
    fat_get_fs_info(&fi);
    assert(fi.mounted == 1);
    assert(fi.volume_start_lba == 2048);
    assert(fat_read_file("mbrtest.txt", readback, 64, &out_sz) == 0);
    assert(out_sz == 17 && memcmp(readback, "MBR_MOUNT_SUCCESS", 17) == 0);

    free(pattern);
    free(readback);
    printf("[PASS] FAT12, FAT16, MBR mounting & zero-leak cluster management\n");
}

static void test_network_stack(void) {
    memset(&nic_state, 0, sizeof(nic_state));
    nic_state.initialized = 1;
    nic_state.link_up = 1;
    nic_state.mac[0] = 0x52; nic_state.mac[1] = 0x54; nic_state.mac[2] = 0x00;
    nic_state.mac[3] = 0x12; nic_state.mac[4] = 0x34; nic_state.mac[5] = 0x56;

    net_init();

    /* IPv4 parse & format */
    uint32_t ip = 0;
    assert(net_parse_ip("10.0.2.15", &ip) == 0);
    assert(ip == 0x0A00020FU);
    assert(net_parse_ip("999.1.1.1", &ip) != 0);
    assert(net_parse_ip("10.0.2", &ip) != 0);

    char ip_s[20];
    net_format_ip(0xC0A80101U, ip_s);
    assert(strcmp(ip_s, "192.168.1.1") == 0);

    /* RFC 1071 Checksum verification */
    uint8_t sample_ip_hdr[20] = {
        0x45, 0x00, 0x00, 0x3c, 0x1c, 0x46, 0x40, 0x00,
        0x40, 0x06, 0x00, 0x00, 0xac, 0x10, 0x0a, 0x63,
        0xac, 0x10, 0x0a, 0x0c
    };
    uint16_t cs = net_checksum(sample_ip_hdr, 20);
    assert(cs == 0xB1E6);
    sample_ip_hdr[10] = (uint8_t)(cs >> 8);
    sample_ip_hdr[11] = (uint8_t)(cs & 0xFF);
    assert(net_checksum(sample_ip_hdr, 20) == 0);

    /* Full 4-way DHCPv4 DORA */
    assert(net_dhcp_discover(500) == 0);
    assert(net_config.configured == 1);
    assert(net_config.dhcp_leased == 1);
    assert(net_config.ip == net_make_ip(10, 0, 2, 15));
    assert(net_config.netmask == net_make_ip(255, 255, 255, 0));
    assert(net_config.gateway == net_make_ip(10, 0, 2, 2));
    assert(net_config.dns == net_make_ip(10, 0, 2, 3));

    /* Outbound ARP + ICMP Ping */
    uint32_t rtt = 999;
    uint8_t ttl = 0;
    assert(net_ping(net_make_ip(10, 0, 2, 2), 1, 500, &rtt, &ttl) == 0);
    assert(ttl == 64);

    /* Inbound ICMP Echo Request -> verify automatic Echo Reply */
    uint8_t in_ping[50];
    memset(in_ping, 0, sizeof(in_ping));
    memcpy(in_ping + 0, nic_state.mac, 6);
    memcpy(in_ping + 6, gw_mac, 6);
    in_ping[12] = 0x08; in_ping[13] = 0x00;
    in_ping[14] = 0x45; in_ping[17] = 36; in_ping[22] = 64; in_ping[23] = 1;
    in_ping[26] = 10; in_ping[27] = 0; in_ping[28] = 2; in_ping[29] = 2;
    in_ping[30] = 10; in_ping[31] = 0; in_ping[32] = 2; in_ping[33] = 15;
    uint16_t ics = htons(net_checksum(in_ping + 14, 20));
    memcpy(in_ping + 24, &ics, 2);
    in_ping[34] = 8; /* Echo Request */
    in_ping[38] = 0x12; in_ping[39] = 0x34;
    in_ping[40] = 0x00; in_ping[41] = 0x07;
    memcpy(in_ping + 42, "PINGTEST", 8);
    uint16_t p_cs = htons(net_checksum(in_ping + 34, 16));
    memcpy(in_ping + 36, &p_cs, 2);

    last_tx_len = 0;
    net_rx_packet(in_ping, sizeof(in_ping));
    assert(last_tx_len == sizeof(in_ping));
    assert(last_tx_frame[34] == 0); /* Echo Reply! */
    assert(net_checksum(last_tx_frame + 34, 16) == 0);

    printf("[PASS] TCP/IP stack: RFC 1071 checksum, ARP, DHCPv4 DORA & ICMP ping\n");
}

int main(void) {
    printf("=== Bengal Tiger OS Host Verification Suite ===\n");
    test_common_and_klog();
    test_heap_allocator();
    test_fat12_fat16_and_mbr();
    test_network_stack();
    printf("=== ALL HOST UNIT & SUBSYSTEM TESTS PASSED ===\n");
    return 0;
}
