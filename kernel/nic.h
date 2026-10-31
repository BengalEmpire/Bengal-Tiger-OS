/**
 * Bengal Tiger OS - Network Interface Controller (NIC) Driver
 *
 * Dual-driver architecture supporting:
 *   1. Intel PRO/1000 (E1000: 82540EM / 82545EM / 82574L) — QEMU, VirtualBox, VMware default
 *   2. Realtek RTL8139 Fast Ethernet — Classic PCI Ethernet
 *
 * Uses static identity-mapped DMA rings (guaranteed physical == virtual below 32MB)
 * and supports both PCI IRQ reception and lock-free polling.
 *
 * @file nic.h
 * @author Bengal Tiger OS (BengalEmpire)
 * @version 1.0.0
 */

#ifndef NIC_H
#define NIC_H

#include "common.h"

#define NIC_NONE    0
#define NIC_E1000   1
#define NIC_RTL8139 2

/* Intel E1000 PCI IDs */
#define INTEL_VENDOR_ID         0x8086
#define E1000_DEV_82540EM       0x100E
#define E1000_DEV_82545EM       0x100F
#define E1000_DEV_82574L        0x10D3

/* Realtek RTL8139 PCI IDs */
#define RTL8139_VENDOR_ID       0x10EC
#define RTL8139_DEVICE_ID       0x8139

typedef struct {
    int      initialized;
    int      driver_type;       /* NIC_NONE, NIC_E1000, NIC_RTL8139 */
    char     driver_name[24];
    uint16_t io_base;
    uint32_t mmio_phys;
    volatile uint8_t *mmio_base;
    uint8_t  irq;
    uint8_t  mac[6];
    int      link_up;
    uint32_t tx_packets;
    uint32_t rx_packets;
    uint32_t tx_bytes;
    uint32_t rx_bytes;
} nic_state_t;

extern nic_state_t nic_state;

int  nic_init(void);
int  nic_send_packet(const void *data, uint16_t length);
void nic_poll(void);
void nic_handle_irq(uint8_t irq);
void nic_get_mac(uint8_t *mac_out);

#endif /* NIC_H */
