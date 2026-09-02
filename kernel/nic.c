/**
 * Bengal Tiger OS - Realtek RTL8139 Network Interface Controller Driver
 *
 * Supports PCI discovery, hardware initialization, MAC retrieval,
 * ring-buffer packet transmission (TX) and reception (RX).
 * 
 * @file nic.c
 * @author Bengal Tiger OS (BengalEmpire)
 * @version 0.6.0
 */

#include "nic.h"
#include "common.h"
#include "pci.h"
#include "heap.h"
#include "serial.h"

/* Network interfaces */
static nic_t interfaces[4];
static int nic_count = 0;

/* Realtek 8139 I/O Register Offsets */
#define RTL8139_REG_MAC0        0x00
#define RTL8139_REG_MAR0        0x08
#define RTL8139_REG_TSD0        0x10
#define RTL8139_REG_TSAD0       0x20
#define RTL8139_REG_RBSTART     0x30
#define RTL8139_REG_CR          0x37
#define RTL8139_REG_CAPR        0x38
#define RTL8139_REG_CBR         0x3A
#define RTL8139_REG_IMR         0x3C
#define RTL8139_REG_ISR         0x3E
#define RTL8139_REG_TCR         0x40
#define RTL8139_REG_RCR         0x44
#define RTL8139_REG_CONFIG1     0x52

#define RX_BUFFER_SIZE          8192
#define RX_BUFFER_PAD           16
#define RX_BUFFER_TOTAL         (RX_BUFFER_SIZE + RX_BUFFER_PAD + 1500)

/* Internal buffer state */
static uint8_t *rx_buffer = NULL;
static uint32_t rx_offset = 0;
static uint8_t tx_counter = 0;

static const uint8_t tx_tsd_regs[4] = {0x10, 0x14, 0x18, 0x1C};
static const uint8_t tx_tsad_regs[4] = {0x20, 0x24, 0x28, 0x2C};

static void rtl8139_init(nic_t *nic, pci_device_t *dev) {
    /* Enable PCI Bus Mastering and I/O Space */
    uint32_t pci_cmd = pci_config_read(dev->bus, dev->device, dev->function, 0x04);
    pci_cmd |= 0x05; /* Bit 0 (I/O Space), Bit 2 (Bus Master) */
    pci_config_write(dev->bus, dev->device, dev->function, 0x04, pci_cmd);

    /* Get BAR0 I/O base address */
    nic->io_base = dev->bar[0] & ~0x3;
    nic->irq = dev->irq;

    /* Power on (CONFIG1 = 0x00) */
    outb(nic->io_base + RTL8139_REG_CONFIG1, 0x00);

    /* Software Reset */
    outb(nic->io_base + RTL8139_REG_CR, 0x10);
    while ((inb(nic->io_base + RTL8139_REG_CR) & 0x10) != 0) {
        /* Wait for reset */
    }

    /* Allocate RX Buffer */
    if (!rx_buffer) {
        rx_buffer = (uint8_t*)kmalloc(RX_BUFFER_TOTAL);
        memset(rx_buffer, 0, RX_BUFFER_TOTAL);
    }
    outl(nic->io_base + RTL8139_REG_RBSTART, (uint32_t)rx_buffer);

    /* Read MAC Address */
    for (int i = 0; i < 6; i++) {
        nic->mac[i] = inb(nic->io_base + RTL8139_REG_MAC0 + i);
    }

    /* Set default IP configuration (192.168.1.100) */
    nic->ip_addr = 0xC0A80164;
    nic->netmask = 0xFFFFFF00;
    nic->gateway = 0xC0A80101;

    /* Configure Interrupts (IMR) - Enable Rx OK, Tx OK, Rx Error, Tx Error */
    outw(nic->io_base + RTL8139_REG_IMR, 0x0005);

    /* Configure Receive Options (RCR): AB+AM+APM+AAP (0x0F), WRAP=0, 8K buffer */
    outl(nic->io_base + RTL8139_REG_RCR, 0x0F | (1 << 7));

    /* Enable Transmitter and Receiver in Command Register (CR) */
    outb(nic->io_base + RTL8139_REG_CR, 0x0C);

    nic->status = NIC_STATUS_UP;

    serial_write_str("RTL8139: Driver initialized on I/O 0x");
    serial_write_hex(COM1_PORT, nic->io_base);
    serial_write_str("\n");
}

void nic_init(void) {
    nic_count = 0;
    memset(interfaces, 0, sizeof(interfaces));

    pci_device_t *rtl_dev = pci_find_device(0x10EC, 0x8139);
    if (rtl_dev) {
        nic_t *nic = &interfaces[0];
        strcpy(nic->name, "eth0");
        rtl8139_init(nic, rtl_dev);
        nic_count = 1;
    } else {
        /* Check generic PCI class 0x02 */
        uint32_t pci_count = pci_get_device_count();
        for (uint32_t i = 0; i < pci_count && nic_count < 4; i++) {
            pci_device_t *dev = pci_get_device(i);
            if (!dev) continue;
            if (dev->class_code == 0x02) {
                nic_t *nic = &interfaces[nic_count];
                nic->name[0] = 'e';
                nic->name[1] = 't';
                nic->name[2] = 'h';
                nic->name[3] = '0' + nic_count;
                nic->name[4] = 0;
                nic->status = NIC_STATUS_DOWN;
                nic_count++;
            }
        }
    }
}

nic_t* nic_get_interface(int index) {
    if (index < 0 || index >= nic_count) return NULL;
    return &interfaces[index];
}

int nic_get_count(void) {
    return nic_count;
}

int nic_send(nic_t *nic, uint8_t *dest_mac, uint16_t type, void *data, uint32_t len) {
    if (!nic || nic->status != NIC_STATUS_UP || !dest_mac || !data) return -1;

    uint32_t total_len = 14 + len;
    if (total_len < 60) total_len = 60; /* Minimum Ethernet frame length */

    uint8_t *frame = (uint8_t*)kmalloc(total_len);
    if (!frame) return -1;

    memset(frame, 0, total_len);

    /* Destination MAC */
    memcpy(frame, dest_mac, 6);
    /* Source MAC */
    memcpy(frame + 6, nic->mac, 6);
    /* EtherType (Big Endian) */
    frame[12] = (type >> 8) & 0xFF;
    frame[13] = type & 0xFF;
    /* Payload */
    memcpy(frame + 14, data, len);

    uint8_t descriptor = tx_counter % 4;
    tx_counter++;

    /* Set physical address for TX descriptor */
    outl(nic->io_base + tx_tsad_regs[descriptor], (uint32_t)frame);
    /* Set length and trigger transmit */
    outl(nic->io_base + tx_tsd_regs[descriptor], total_len & 0x1FFF);

    nic->tx_packets++;
    nic->tx_bytes += total_len;

    /* Frame freed after transmission trigger */
    kfree(frame);
    return 0;
}

int nic_receive(nic_t *nic, void *buffer, uint32_t max_len) {
    if (!nic || nic->status != NIC_STATUS_UP || !buffer || !rx_buffer) return -1;

    /* Check if CR is not empty */
    if (inb(nic->io_base + RTL8139_REG_CR) & 0x01) {
        return -1; /* Buffer empty */
    }

    uint16_t *header = (uint16_t*)(rx_buffer + rx_offset);
    uint16_t rx_status = header[0];
    uint16_t rx_len = header[1];

    if (!(rx_status & 0x01) || rx_len < 4) {
        return -1; /* Packet error / empty */
    }

    uint32_t payload_len = (uint32_t)(rx_len - 4);
    uint32_t copy_len = (payload_len < max_len) ? payload_len : max_len;
    memcpy(buffer, rx_buffer + rx_offset + 4, copy_len);

    /* Update CAPR (Current Address Pointer Register) */
    rx_offset = (rx_offset + rx_len + 4 + 3) & ~3;
    if (rx_offset >= RX_BUFFER_SIZE) {
        rx_offset %= RX_BUFFER_SIZE;
    }
    outw(nic->io_base + RTL8139_REG_CAPR, rx_offset - 0x10);

    nic->rx_packets++;
    nic->rx_bytes += copy_len;

    return (int)copy_len;
}

void nic_print_mac(uint8_t *mac) {
    if (!mac) return;
    /* Formatting handled in shell layer */
}
