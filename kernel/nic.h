/**
 * Bengal Tiger OS - Network Interface Controller (Stub)
 * 
 * Placeholder for future network driver implementation.
 * Will support Intel E1000 and RTL8139 NICs.
 * 
 * @file nic.h
 * @author Bengal Tiger OS (BengalEmpire)
 * @version 0.3.0
 */

#ifndef NIC_H
#define NIC_H

#include "common.h"

/* NIC Status */
#define NIC_STATUS_DOWN     0
#define NIC_STATUS_UP       1
#define NIC_STATUS_ERROR    2

/* MAC Address size */
#define MAC_SIZE 6

/* Network Interface Structure */
typedef struct {
    char name[16];              /* Interface name (e.g., "eth0") */
    uint8_t mac[MAC_SIZE];      /* MAC address */
    uint32_t ip_addr;           /* IP address (if configured) */
    uint32_t netmask;           /* Subnet mask */
    uint32_t gateway;           /* Default gateway */
    uint8_t status;             /* Interface status */
    
    /* Statistics */
    uint32_t tx_packets;        /* Packets transmitted */
    uint32_t rx_packets;        /* Packets received */
    uint32_t tx_bytes;          /* Bytes transmitted */
    uint32_t rx_bytes;          /* Bytes received */
    uint32_t tx_errors;         /* Transmit errors */
    uint32_t rx_errors;         /* Receive errors */
} nic_t;

/**
 * Initialize network drivers.
 * Scans PCI bus for supported NICs.
 */
void nic_init(void);

/**
 * Get network interface by index.
 * @param index Interface index
 * @return Pointer to NIC structure, or NULL
 */
nic_t* nic_get_interface(int index);

/**
 * Get number of available NICs.
 */
int nic_get_count(void);

/**
 * Send an Ethernet frame (stub).
 * 
 * @param nic Network interface
 * @param dest_mac Destination MAC address
 * @param type Ethernet type (0x0800 = IPv4, 0x0806 = ARP)
 * @param data Frame payload
 * @param len Payload length
 * @return 0 on success, -1 on error
 */
int nic_send(nic_t *nic, uint8_t *dest_mac, uint16_t type, void *data, uint32_t len);

/**
 * Receive an Ethernet frame (stub).
 * 
 * @param nic Network interface
 * @param buffer Receive buffer
 * @param max_len Maximum length to receive
 * @return Number of bytes received, or -1 on error
 */
int nic_receive(nic_t *nic, void *buffer, uint32_t max_len);

/**
 * Print MAC address in human-readable format.
 */
void nic_print_mac(uint8_t *mac);

#endif /* NIC_H */
