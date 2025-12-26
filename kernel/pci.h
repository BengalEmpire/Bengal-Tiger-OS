/**
 * Bengal Tiger OS - PCI Bus Driver
 * 
 * PCI Configuration Space access and device enumeration.
 * 
 * @file pci.h
 * @author Bengal Tiger OS (BengalEmpire)
 * @version 0.3.0
 */

#ifndef PCI_H
#define PCI_H

#include "common.h"

/* PCI Configuration Space Ports */
#define PCI_CONFIG_ADDRESS  0xCF8
#define PCI_CONFIG_DATA     0xCFC
#define PCI_CONFIG_ENABLE   0x80000000

/* Maximum devices to track */
#define PCI_MAX_DEVICES     64

/* PCI Device Structure */
typedef struct {
    uint8_t bus;
    uint8_t device;
    uint8_t function;
    uint16_t vendor_id;
    uint16_t device_id;
    uint8_t class_code;
    uint8_t subclass;
    uint8_t prog_if;
    uint8_t revision;
    uint8_t header_type;
    uint8_t irq;
    uint32_t bar[6];        /* Base Address Registers */
} pci_device_t;

/* 32-bit I/O (needed for PCI) */
static inline void outl(uint16_t port, uint32_t val) {
    __asm__ volatile("outl %0, %1" : : "a"(val), "Nd"(port));
}

static inline uint32_t inl(uint16_t port) {
    uint32_t ret;
    __asm__ volatile("inl %1, %0" : "=a"(ret) : "Nd"(port));
    return ret;
}

/**
 * Initialize PCI subsystem - scans all buses for devices.
 */
void pci_init(void);

/**
 * Read from PCI configuration space.
 * @param bus Bus number (0-255)
 * @param device Device number (0-31)
 * @param func Function number (0-7)
 * @param offset Register offset (must be 4-byte aligned)
 * @return 32-bit value from configuration space
 */
uint32_t pci_config_read(uint8_t bus, uint8_t device, uint8_t func, uint8_t offset);

/**
 * Write to PCI configuration space.
 */
void pci_config_write(uint8_t bus, uint8_t device, uint8_t func, uint8_t offset, uint32_t value);

/**
 * Get number of detected PCI devices.
 */
uint32_t pci_get_device_count(void);

/**
 * Get device by index.
 * @param index Device index (0 to count-1)
 * @return Pointer to device structure, or NULL if invalid
 */
pci_device_t* pci_get_device(uint32_t index);

/**
 * Find device by vendor and device ID.
 * @return Pointer to first matching device, or NULL
 */
pci_device_t* pci_find_device(uint16_t vendor_id, uint16_t device_id);

/**
 * Find device by class and subclass.
 * @return Pointer to first matching device, or NULL
 */
pci_device_t* pci_find_device_by_class(uint8_t class_code, uint8_t subclass);

/**
 * Get human-readable vendor name.
 */
const char* pci_vendor_name(uint16_t vendor_id);

/**
 * Get human-readable class name.
 */
const char* pci_class_name(uint8_t class_code);

/**
 * List all PCI devices (prints to shell).
 */
void pci_list_devices(void);

#endif /* PCI_H */
