/**
 * Bengal Tiger OS - Network Interface Controller (Stub)
 * 
 * @file nic.c
 * @author Bengal Tiger OS (BengalEmpire)
 * @version 0.3.0
 */

#include "nic.h"
#include "common.h"
#include "pci.h"

/* Network interfaces */
static nic_t interfaces[4];
static int nic_count = 0;

/* Known NIC vendor/device IDs */
#define INTEL_VENDOR    0x8086
#define E1000_DEVICE    0x100E  /* Intel 82540EM (QEMU default) */
#define E1000E_DEVICE   0x10D3  /* Intel 82574L */

#define REALTEK_VENDOR  0x10EC
#define RTL8139_DEVICE  0x8139  /* Realtek RTL8139 */

#define VIRTIO_VENDOR   0x1AF4
#define VIRTIO_NET      0x1000  /* VirtIO network */

void nic_init(void) {
    nic_count = 0;
    
    /* Clear interface array */
    memset(interfaces, 0, sizeof(interfaces));
    
    /* Search for network controllers in PCI devices */
    uint32_t pci_count = pci_get_device_count();
    
    for (uint32_t i = 0; i < pci_count && nic_count < 4; i++) {
        pci_device_t *dev = pci_get_device(i);
        if (!dev) continue;
        
        /* Check if it's a network controller (class 0x02) */
        if (dev->class_code == 0x02) {
            nic_t *nic = &interfaces[nic_count];
            
            /* Set interface name */
            nic->name[0] = 'e';
            nic->name[1] = 't';
            nic->name[2] = 'h';
            nic->name[3] = '0' + nic_count;
            nic->name[4] = 0;
            
            /* Set status as down (no actual driver) */
            nic->status = NIC_STATUS_DOWN;
            
            /* Zero MAC address (would be read from device) */
            memset(nic->mac, 0, MAC_SIZE);
            
            /* Initialize statistics */
            nic->tx_packets = 0;
            nic->rx_packets = 0;
            nic->tx_bytes = 0;
            nic->rx_bytes = 0;
            nic->tx_errors = 0;
            nic->rx_errors = 0;
            
            /* 
             * TODO: Actual driver initialization based on device
             * 
             * if (dev->vendor_id == INTEL_VENDOR && dev->device_id == E1000_DEVICE) {
             *     e1000_init(nic, dev);
             * } else if (dev->vendor_id == REALTEK_VENDOR && dev->device_id == RTL8139_DEVICE) {
             *     rtl8139_init(nic, dev);
             * }
             */
            
            nic_count++;
        }
    }
}

nic_t* nic_get_interface(int index) {
    if (index < 0 || index >= nic_count) {
        return NULL;
    }
    return &interfaces[index];
}

int nic_get_count(void) {
    return nic_count;
}

int nic_send(nic_t *nic, uint8_t *dest_mac, uint16_t type, void *data, uint32_t len) {
    /* Stub: Would send Ethernet frame */
    if (!nic || !dest_mac || !data) return -1;
    
    UNUSED(type);
    UNUSED(len);
    
    /* Not implemented */
    return -1;
}

int nic_receive(nic_t *nic, void *buffer, uint32_t max_len) {
    /* Stub: Would receive Ethernet frame */
    if (!nic || !buffer) return -1;
    
    UNUSED(max_len);
    
    /* Not implemented */
    return -1;
}

void nic_print_mac(uint8_t *mac) {
    /* Would print MAC as XX:XX:XX:XX:XX:XX */
    UNUSED(mac);
}