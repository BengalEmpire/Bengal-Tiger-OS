/**
 * Bengal Tiger OS - Network Interface Controller (E1000 + RTL8139) Implementation
 *
 * @file nic.c
 * @author Bengal Tiger OS (BengalEmpire)
 * @version 1.0.0
 */

#include "nic.h"
#include "pci.h"
#include "paging.h"
#include "pic.h"
#include "net.h"
#include "klog.h"
#include "common.h"

nic_state_t nic_state;

/* =========================================================================
 * Intel E1000 Registers & Static Identity-Mapped DMA Rings
 * ========================================================================= */
#define E1000_CTRL      0x0000
#define E1000_STATUS    0x0008
#define E1000_EERD      0x0014
#define E1000_ICR       0x00C0
#define E1000_IMS       0x00D0
#define E1000_IMC       0x00D8
#define E1000_RCTL      0x0100
#define E1000_TCTL      0x0400
#define E1000_TIPG      0x0410
#define E1000_RDBAL     0x2800
#define E1000_RDBAH     0x2804
#define E1000_RDLEN     0x2808
#define E1000_RDH       0x2810
#define E1000_RDT       0x2818
#define E1000_TDBAL     0x3800
#define E1000_TDBAH     0x3804
#define E1000_TDLEN     0x3808
#define E1000_TDH       0x3810
#define E1000_TDT       0x3818
#define E1000_MTA       0x5200
#define E1000_RAL0      0x5400
#define E1000_RAH0      0x5404

#define E1000_CTRL_SLU  (1U << 6)
#define E1000_CTRL_RST  (1U << 26)

#define E1000_RCTL_EN           (1U << 1)
#define E1000_RCTL_SBP          (1U << 2)
#define E1000_RCTL_UPE          (1U << 3)
#define E1000_RCTL_MPE          (1U << 4)
#define E1000_RCTL_BAM          (1U << 15)
#define E1000_RCTL_BSIZE_2048   (0U << 16)
#define E1000_RCTL_SECRC        (1U << 26)

#define E1000_TCTL_EN           (1U << 1)
#define E1000_TCTL_PSP          (1U << 3)

#define E1000_TX_CMD_EOP        0x01
#define E1000_TX_CMD_IFCS       0x02
#define E1000_TX_CMD_RS         0x08

#define E1000_NUM_RX_DESC       32
#define E1000_NUM_TX_DESC       8
#define E1000_PKT_BUF_SIZE      2048

typedef struct {
    uint64_t addr;
    uint16_t length;
    uint16_t checksum;
    uint8_t  status;
    uint8_t  errors;
    uint16_t special;
} __attribute__((packed)) e1000_rx_desc_t;

typedef struct {
    uint64_t addr;
    uint16_t length;
    uint8_t  cso;
    uint8_t  cmd;
    uint8_t  status;
    uint8_t  css;
    uint16_t special;
} __attribute__((packed)) e1000_tx_desc_t;

static volatile e1000_rx_desc_t e1000_rx_descs[E1000_NUM_RX_DESC] __attribute__((aligned(16)));
static volatile e1000_tx_desc_t e1000_tx_descs[E1000_NUM_TX_DESC] __attribute__((aligned(16)));
static uint8_t e1000_rx_bufs[E1000_NUM_RX_DESC][E1000_PKT_BUF_SIZE] __attribute__((aligned(16)));
static uint8_t e1000_tx_bufs[E1000_NUM_TX_DESC][E1000_PKT_BUF_SIZE] __attribute__((aligned(16)));
static uint32_t e1000_rx_cur = 0;
static uint32_t e1000_tx_cur = 0;

static inline void e1000_write32(uint32_t reg, uint32_t val) {
    *(volatile uint32_t*)(nic_state.mmio_base + reg) = val;
}

static inline uint32_t e1000_read32(uint32_t reg) {
    return *(volatile uint32_t*)(nic_state.mmio_base + reg);
}

static int e1000_read_eeprom(uint8_t addr, uint16_t *out) {
    e1000_write32(E1000_EERD, 1U | ((uint32_t)addr << 8));
    for (int i = 0; i < 10000; i++) {
        uint32_t v = e1000_read32(E1000_EERD);
        if (v & (1U << 4)) {
            *out = (uint16_t)(v >> 16);
            return 0;
        }
    }
    return -1;
}

static int e1000_init_device(pci_device_t *dev) {
    uint32_t bar0 = dev->bar[0];
    if (bar0 & 0x01) {
        return -1; /* Expected MMIO BAR0 */
    }

    uint32_t mmio_phys = bar0 & ~0xFU;
    if (mmio_phys == 0) return -1;

    /* Map 128KB MMIO register space uncached */
    paging_map_region(mmio_phys, mmio_phys, 128 * 1024,
                      PAGE_PRESENT | PAGE_WRITABLE | PAGE_PCD | PAGE_PWT);

    /* Enable PCI Bus Mastering + Memory Space + I/O Space */
    uint32_t cmd = pci_config_read(dev->bus, dev->device, dev->function, 0x04);
    pci_config_write(dev->bus, dev->device, dev->function, 0x04, cmd | 0x07);

    nic_state.driver_type = NIC_E1000;
    strcpy(nic_state.driver_name, "Intel PRO/1000 (E1000)");
    nic_state.mmio_phys = mmio_phys;
    nic_state.mmio_base = (volatile uint8_t*)(uintptr_t)mmio_phys;
    nic_state.irq       = dev->irq;

    /* Mask interrupts and reset controller */
    e1000_write32(E1000_IMC, 0xFFFFFFFFU);
    e1000_write32(E1000_CTRL, e1000_read32(E1000_CTRL) | E1000_CTRL_RST);
    for (volatile int i = 0; i < 100000; i++);
    e1000_write32(E1000_IMC, 0xFFFFFFFFU);
    (void)e1000_read32(E1000_ICR);

    /* Set Link Up */
    uint32_t ctrl = e1000_read32(E1000_CTRL);
    ctrl |= E1000_CTRL_SLU;
    ctrl &= ~(1U << 3);  /* Clear LRST */
    ctrl &= ~(1U << 31); /* Clear PHY_RST */
    e1000_write32(E1000_CTRL, ctrl);

    /* Clear Multicast Table Array */
    for (int i = 0; i < 128; i++) {
        e1000_write32(E1000_MTA + (uint32_t)(i * 4), 0);
    }

    /* Read MAC address from EEPROM or RAL0/RAH0 */
    uint16_t w0 = 0, w1 = 0, w2 = 0;
    if (e1000_read_eeprom(0, &w0) == 0 &&
        e1000_read_eeprom(1, &w1) == 0 &&
        e1000_read_eeprom(2, &w2) == 0 &&
        (w0 | w1 | w2) != 0) {
        nic_state.mac[0] = (uint8_t)(w0 & 0xFF);
        nic_state.mac[1] = (uint8_t)(w0 >> 8);
        nic_state.mac[2] = (uint8_t)(w1 & 0xFF);
        nic_state.mac[3] = (uint8_t)(w1 >> 8);
        nic_state.mac[4] = (uint8_t)(w2 & 0xFF);
        nic_state.mac[5] = (uint8_t)(w2 >> 8);
    } else {
        uint32_t ral = e1000_read32(E1000_RAL0);
        uint32_t rah = e1000_read32(E1000_RAH0);
        nic_state.mac[0] = (uint8_t)(ral & 0xFF);
        nic_state.mac[1] = (uint8_t)((ral >> 8) & 0xFF);
        nic_state.mac[2] = (uint8_t)((ral >> 16) & 0xFF);
        nic_state.mac[3] = (uint8_t)((ral >> 24) & 0xFF);
        nic_state.mac[4] = (uint8_t)(rah & 0xFF);
        nic_state.mac[5] = (uint8_t)((rah >> 8) & 0xFF);
    }

    /* Program MAC into RAL0/RAH0 with Address Valid (AV) bit */
    uint32_t ral_prog = (uint32_t)nic_state.mac[0] |
                        ((uint32_t)nic_state.mac[1] << 8) |
                        ((uint32_t)nic_state.mac[2] << 16) |
                        ((uint32_t)nic_state.mac[3] << 24);
    uint32_t rah_prog = (uint32_t)nic_state.mac[4] |
                        ((uint32_t)nic_state.mac[5] << 8) |
                        (1U << 31);
    e1000_write32(E1000_RAL0, ral_prog);
    e1000_write32(E1000_RAH0, rah_prog);

    /* Setup RX Descriptor Ring */
    for (int i = 0; i < E1000_NUM_RX_DESC; i++) {
        e1000_rx_descs[i].addr     = (uint64_t)(uintptr_t)e1000_rx_bufs[i];
        e1000_rx_descs[i].length   = 0;
        e1000_rx_descs[i].checksum = 0;
        e1000_rx_descs[i].status   = 0;
        e1000_rx_descs[i].errors   = 0;
        e1000_rx_descs[i].special  = 0;
    }
    e1000_rx_cur = 0;
    e1000_write32(E1000_RDBAL, (uint32_t)(uintptr_t)e1000_rx_descs);
    e1000_write32(E1000_RDBAH, 0);
    e1000_write32(E1000_RDLEN, sizeof(e1000_rx_descs));
    e1000_write32(E1000_RDH, 0);
    e1000_write32(E1000_RDT, E1000_NUM_RX_DESC - 1);
    e1000_write32(E1000_RCTL,
                  E1000_RCTL_EN | E1000_RCTL_SBP | E1000_RCTL_BAM |
                  E1000_RCTL_BSIZE_2048 | E1000_RCTL_SECRC);

    /* Setup TX Descriptor Ring */
    for (int i = 0; i < E1000_NUM_TX_DESC; i++) {
        e1000_tx_descs[i].addr    = (uint64_t)(uintptr_t)e1000_tx_bufs[i];
        e1000_tx_descs[i].length  = 0;
        e1000_tx_descs[i].cso     = 0;
        e1000_tx_descs[i].cmd     = 0;
        e1000_tx_descs[i].status  = 0x01; /* DD = descriptor available */
        e1000_tx_descs[i].css     = 0;
        e1000_tx_descs[i].special = 0;
    }
    e1000_tx_cur = 0;
    e1000_write32(E1000_TDBAL, (uint32_t)(uintptr_t)e1000_tx_descs);
    e1000_write32(E1000_TDBAH, 0);
    e1000_write32(E1000_TDLEN, sizeof(e1000_tx_descs));
    e1000_write32(E1000_TDH, 0);
    e1000_write32(E1000_TDT, 0);
    e1000_write32(E1000_TIPG, 0x0060200AU);
    e1000_write32(E1000_TCTL,
                  E1000_TCTL_EN | E1000_TCTL_PSP | (15U << 4) | (0x40U << 12));

    /* Enable RX/Link interrupts */
    e1000_write32(E1000_IMS, 0x80U | 0x04U | 0x40U | 0x10U);
    if (nic_state.irq > 0 && nic_state.irq < 16) {
        if (nic_state.irq >= 8) pic_unmask_irq(2);
        pic_unmask_irq(nic_state.irq);
    }

    nic_state.link_up     = (e1000_read32(E1000_STATUS) & 0x02) ? 1 : 1;
    nic_state.initialized = 1;
    return 0;
}

static int e1000_send(const void *data, uint16_t length) {
    if (length > 1518) return -1;

    uint32_t idx = e1000_tx_cur;
    int timeout = 100000;
    while (!(e1000_tx_descs[idx].status & 0x01) && timeout-- > 0);
    if (timeout <= 0) return -1;

    uint16_t send_len = (length < 60) ? 60 : length;
    memset(e1000_tx_bufs[idx], 0, send_len);
    memcpy(e1000_tx_bufs[idx], data, length);

    e1000_tx_descs[idx].addr   = (uint64_t)(uintptr_t)e1000_tx_bufs[idx];
    e1000_tx_descs[idx].length = send_len;
    e1000_tx_descs[idx].cmd    = E1000_TX_CMD_EOP | E1000_TX_CMD_IFCS | E1000_TX_CMD_RS;
    e1000_tx_descs[idx].status = 0;

    e1000_tx_cur = (idx + 1) % E1000_NUM_TX_DESC;
    e1000_write32(E1000_TDT, e1000_tx_cur);

    nic_state.tx_packets++;
    nic_state.tx_bytes += send_len;
    return 0;
}

static void e1000_poll_rx(void) {
    int budget = 32;
    while ((e1000_rx_descs[e1000_rx_cur].status & 0x01) && budget-- > 0) {
        uint8_t  err = e1000_rx_descs[e1000_rx_cur].errors;
        uint16_t len = e1000_rx_descs[e1000_rx_cur].length;

        /* Copy frame to local stack buffer before recycling descriptor */
        uint8_t pkt[1600];
        uint16_t copy_len = (len <= sizeof(pkt)) ? len : 0;
        if (err == 0 && copy_len >= 14) {
            memcpy(pkt, e1000_rx_bufs[e1000_rx_cur], copy_len);
        } else {
            copy_len = 0;
        }

        uint32_t old_cur = e1000_rx_cur;
        e1000_rx_descs[old_cur].status = 0;
        e1000_rx_cur = (old_cur + 1) % E1000_NUM_RX_DESC;
        e1000_write32(E1000_RDT, old_cur);

        if (copy_len >= 14) {
            nic_state.rx_packets++;
            nic_state.rx_bytes += copy_len;
            net_rx_packet(pkt, copy_len);
        }
    }
}

/* =========================================================================
 * Realtek RTL8139 Static Identity-Mapped DMA Driver
 * ========================================================================= */
#define RTL_MAC0        0x00
#define RTL_TSD0        0x10
#define RTL_TSAD0       0x20
#define RTL_RBSTART     0x30
#define RTL_CMD         0x37
#define RTL_CAPR        0x38
#define RTL_CBR         0x3A
#define RTL_IMR         0x3C
#define RTL_ISR         0x3E
#define RTL_TCR         0x40
#define RTL_RCR         0x44
#define RTL_CONFIG1     0x52

#define RTL_RX_RING_SIZE 8192
static uint8_t  rtl_rx_buf[RTL_RX_RING_SIZE + 16 + 1536] __attribute__((aligned(16)));
static uint8_t  rtl_tx_bufs[4][2048] __attribute__((aligned(16)));
static uint16_t rtl_rx_offset = 0;
static uint8_t  rtl_tx_cur    = 0;

static int rtl8139_init_device(pci_device_t *dev) {
    uint16_t io = (uint16_t)(dev->bar[0] & ~0x3U);
    if (io == 0) return -1;

    uint32_t cmd = pci_config_read(dev->bus, dev->device, dev->function, 0x04);
    pci_config_write(dev->bus, dev->device, dev->function, 0x04, cmd | 0x05);

    nic_state.driver_type = NIC_RTL8139;
    strcpy(nic_state.driver_name, "Realtek RTL8139");
    nic_state.io_base     = io;
    nic_state.irq         = dev->irq;

    /* Power on & reset */
    outb(io + RTL_CONFIG1, 0x00);
    outb(io + RTL_CMD, 0x10);
    for (int i = 0; i < 100000 && (inb(io + RTL_CMD) & 0x10); i++);

    for (int i = 0; i < 6; i++) {
        nic_state.mac[i] = inb(io + RTL_MAC0 + i);
    }

    memset(rtl_rx_buf, 0, sizeof(rtl_rx_buf));
    rtl_rx_offset = 0;
    rtl_tx_cur    = 0;

    outl(io + RTL_RBSTART, (uint32_t)(uintptr_t)rtl_rx_buf);
    outw(io + RTL_IMR, 0x0005); /* ROK | TOK */
    outl(io + RTL_RCR, 0x0000008FU); /* AAP|APM|AM|AB|WRAP */
    outb(io + RTL_CMD, 0x0C);        /* RE | TE */

    if (nic_state.irq > 0 && nic_state.irq < 16) {
        if (nic_state.irq >= 8) pic_unmask_irq(2);
        pic_unmask_irq(nic_state.irq);
    }

    nic_state.link_up     = 1;
    nic_state.initialized = 1;
    return 0;
}

static int rtl8139_send(const void *data, uint16_t length) {
    if (length > 1518) return -1;
    uint16_t send_len = (length < 60) ? 60 : length;

    uint8_t slot = rtl_tx_cur;
    memset(rtl_tx_bufs[slot], 0, send_len);
    memcpy(rtl_tx_bufs[slot], data, length);

    outl(nic_state.io_base + RTL_TSAD0 + slot * 4, (uint32_t)(uintptr_t)rtl_tx_bufs[slot]);
    outl(nic_state.io_base + RTL_TSD0  + slot * 4, send_len);

    rtl_tx_cur = (slot + 1) & 0x03;
    nic_state.tx_packets++;
    nic_state.tx_bytes += send_len;
    return 0;
}

static void rtl8139_poll_rx(void) {
    uint16_t io = nic_state.io_base;
    int budget = 32;

    while ((inb(io + RTL_CMD) & 0x01) == 0 && budget-- > 0) {
        uint8_t *hdr = rtl_rx_buf + rtl_rx_offset;
        uint16_t status = (uint16_t)hdr[0] | ((uint16_t)hdr[1] << 8);
        uint16_t pkt_len = (uint16_t)hdr[2] | ((uint16_t)hdr[3] << 8);

        if ((status & 0x01) == 0 || pkt_len < 18 || pkt_len > 1536) {
            /* Reset RX pointer on corrupted frame */
            rtl_rx_offset = inw(io + RTL_CBR) % RTL_RX_RING_SIZE;
            outw(io + RTL_CAPR, (uint16_t)(rtl_rx_offset - 16));
            break;
        }

        uint16_t frame_len = pkt_len - 4; /* Strip 4-byte CRC */
        uint8_t pkt[1536];
        memcpy(pkt, hdr + 4, frame_len);

        rtl_rx_offset = (uint16_t)((rtl_rx_offset + pkt_len + 4 + 3) & ~3U);
        rtl_rx_offset %= RTL_RX_RING_SIZE;
        outw(io + RTL_CAPR, (uint16_t)(rtl_rx_offset - 16));

        nic_state.rx_packets++;
        nic_state.rx_bytes += frame_len;
        net_rx_packet(pkt, frame_len);
    }
}

/* =========================================================================
 * Unified NIC Public API
 * ========================================================================= */
int nic_init(void) {
    memset(&nic_state, 0, sizeof(nic_state));
    strcpy(nic_state.driver_name, "None");

    /* 1. Probe Intel E1000 variants (QEMU, VirtualBox, VMware default) */
    const uint16_t e1000_ids[] = { E1000_DEV_82540EM, E1000_DEV_82545EM, E1000_DEV_82574L };
    for (unsigned int i = 0; i < ARRAY_SIZE(e1000_ids); i++) {
        pci_device_t *dev = pci_find_device(INTEL_VENDOR_ID, e1000_ids[i]);
        if (dev && e1000_init_device(dev) == 0) {
            klog_writeln("nic", "Initialized Intel PRO/1000 (E1000) Ethernet");
            return 0;
        }
    }

    /* 2. Probe Realtek RTL8139 */
    pci_device_t *rtl = pci_find_device(RTL8139_VENDOR_ID, RTL8139_DEVICE_ID);
    if (rtl && rtl8139_init_device(rtl) == 0) {
        klog_writeln("nic", "Initialized Realtek RTL8139 Ethernet");
        return 0;
    }

    klog_writeln("nic", "No supported Ethernet controller detected");
    return -1;
}

int nic_send_packet(const void *data, uint16_t length) {
    if (!nic_state.initialized || !data || length == 0) return -1;

    uint32_t flags = irq_save();
    int rc = -1;
    if (nic_state.driver_type == NIC_E1000) {
        rc = e1000_send(data, length);
    } else if (nic_state.driver_type == NIC_RTL8139) {
        rc = rtl8139_send(data, length);
    }
    irq_restore(flags);
    return rc;
}

void nic_poll(void) {
    if (!nic_state.initialized) return;

    uint32_t flags = irq_save();
    if (nic_state.driver_type == NIC_E1000) {
        (void)e1000_read32(E1000_ICR);
        e1000_poll_rx();
    } else if (nic_state.driver_type == NIC_RTL8139) {
        uint16_t isr = inw(nic_state.io_base + RTL_ISR);
        if (isr) outw(nic_state.io_base + RTL_ISR, isr);
        rtl8139_poll_rx();
    }
    irq_restore(flags);
}

void nic_handle_irq(uint8_t irq) {
    if (!nic_state.initialized || irq != nic_state.irq) return;

    if (nic_state.driver_type == NIC_E1000) {
        uint32_t icr = e1000_read32(E1000_ICR);
        if (icr) {
            e1000_poll_rx();
        }
    } else if (nic_state.driver_type == NIC_RTL8139) {
        uint16_t isr = inw(nic_state.io_base + RTL_ISR);
        if (isr) {
            outw(nic_state.io_base + RTL_ISR, isr);
            rtl8139_poll_rx();
        }
    }
}

void nic_get_mac(uint8_t *mac_out) {
    if (!mac_out) return;
    if (!nic_state.initialized) {
        memset(mac_out, 0, 6);
        return;
    }
    memcpy(mac_out, nic_state.mac, 6);
}
