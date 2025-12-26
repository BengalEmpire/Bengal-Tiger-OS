/**
 * Bengal Tiger OS - PCI Bus Scanner
 * 
 * Provides PCI bus enumeration and device detection.
 * Uses Configuration Space Access Mechanism #1 (I/O ports 0xCF8/0xCFC).
 * 
 * @file pci.c
 * @author Bengal Tiger OS (BengalEmpire)
 * @version 0.3.0
 */

#include "pci.h"
#include "common.h"

/* PCI Device Database (simplified) */
static pci_device_t pci_devices[PCI_MAX_DEVICES];
static uint32_t pci_device_count = 0;

/* Known vendor names */
static const char* pci_get_vendor_name(uint16_t vendor_id) {
    switch (vendor_id) {
        case 0x8086: return "Intel";
        case 0x1022: return "AMD";
        case 0x10DE: return "NVIDIA";
        case 0x1002: return "AMD/ATI";
        case 0x10EC: return "Realtek";
        case 0x14E4: return "Broadcom";
        case 0x1AF4: return "Red Hat (VirtIO)";
        case 0x80EE: return "VirtualBox";
        case 0x15AD: return "VMware";
        default: return "Unknown";
    }
}

/* Known class names */
static const char* pci_get_class_name(uint8_t class_code) {
    switch (class_code) {
        case 0x00: return "Unclassified";
        case 0x01: return "Mass Storage Controller";
        case 0x02: return "Network Controller";
        case 0x03: return "Display Controller";
        case 0x04: return "Multimedia Controller";
        case 0x05: return "Memory Controller";
        case 0x06: return "Bridge";
        case 0x07: return "Communication Controller";
        case 0x08: return "System Peripheral";
        case 0x09: return "Input Device";
        case 0x0A: return "Docking Station";
        case 0x0B: return "Processor";
        case 0x0C: return "Serial Bus Controller";
        case 0x0D: return "Wireless Controller";
        case 0x0E: return "Intelligent Controller";
        case 0x0F: return "Satellite";
        case 0x10: return "Encryption Controller";
        case 0x11: return "Signal Processing";
        case 0xFF: return "Unknown";
        default: return "Reserved";
    }
}

uint32_t pci_config_read(uint8_t bus, uint8_t device, uint8_t func, uint8_t offset) {
    uint32_t address;
    
    /* Build configuration address */
    address = (uint32_t)((bus << 16) | (device << 11) | (func << 8) | 
              (offset & 0xFC) | PCI_CONFIG_ENABLE);
    
    /* Write address to CONFIG_ADDRESS port */
    outl(PCI_CONFIG_ADDRESS, address);
    
    /* Read data from CONFIG_DATA port */
    return inl(PCI_CONFIG_DATA);
}

void pci_config_write(uint8_t bus, uint8_t device, uint8_t func, uint8_t offset, uint32_t value) {
    uint32_t address;
    
    address = (uint32_t)((bus << 16) | (device << 11) | (func << 8) | 
              (offset & 0xFC) | PCI_CONFIG_ENABLE);
    
    outl(PCI_CONFIG_ADDRESS, address);
    outl(PCI_CONFIG_DATA, value);
}

static void pci_check_device(uint8_t bus, uint8_t device) {
    uint32_t data;
    uint16_t vendor_id;
    
    /* Read vendor ID from offset 0 */
    data = pci_config_read(bus, device, 0, 0);
    vendor_id = data & 0xFFFF;
    
    /* No device if vendor ID is 0xFFFF */
    if (vendor_id == 0xFFFF) return;
    
    /* Check all 8 possible functions */
    for (uint8_t func = 0; func < 8; func++) {
        data = pci_config_read(bus, device, func, 0);
        vendor_id = data & 0xFFFF;
        
        if (vendor_id == 0xFFFF) continue;
        
        if (pci_device_count >= PCI_MAX_DEVICES) return;
        
        pci_device_t *dev = &pci_devices[pci_device_count];
        
        dev->bus = bus;
        dev->device = device;
        dev->function = func;
        dev->vendor_id = vendor_id;
        dev->device_id = (data >> 16) & 0xFFFF;
        
        /* Read class/subclass/prog_if/revision */
        data = pci_config_read(bus, device, func, 0x08);
        dev->revision = data & 0xFF;
        dev->prog_if = (data >> 8) & 0xFF;
        dev->subclass = (data >> 16) & 0xFF;
        dev->class_code = (data >> 24) & 0xFF;
        
        /* Read header type */
        data = pci_config_read(bus, device, func, 0x0C);
        dev->header_type = (data >> 16) & 0xFF;
        
        /* Read BARs */
        for (int bar = 0; bar < 6; bar++) {
            dev->bar[bar] = pci_config_read(bus, device, func, 0x10 + bar * 4);
        }
        
        /* Read interrupt line */
        data = pci_config_read(bus, device, func, 0x3C);
        dev->irq = data & 0xFF;
        
        pci_device_count++;
        
        /* If not multi-function, no need to check other functions */
        if (func == 0 && !(dev->header_type & 0x80)) break;
    }
}

void pci_init(void) {
    pci_device_count = 0;
    
    /* Scan all buses, devices */
    for (uint16_t bus = 0; bus < 256; bus++) {
        for (uint8_t device = 0; device < 32; device++) {
            pci_check_device((uint8_t)bus, device);
        }
    }
}

uint32_t pci_get_device_count(void) {
    return pci_device_count;
}

pci_device_t* pci_get_device(uint32_t index) {
    if (index >= pci_device_count) return NULL;
    return &pci_devices[index];
}

pci_device_t* pci_find_device(uint16_t vendor_id, uint16_t device_id) {
    for (uint32_t i = 0; i < pci_device_count; i++) {
        if (pci_devices[i].vendor_id == vendor_id && 
            pci_devices[i].device_id == device_id) {
            return &pci_devices[i];
        }
    }
    return NULL;
}

pci_device_t* pci_find_device_by_class(uint8_t class_code, uint8_t subclass) {
    for (uint32_t i = 0; i < pci_device_count; i++) {
        if (pci_devices[i].class_code == class_code && 
            pci_devices[i].subclass == subclass) {
            return &pci_devices[i];
        }
    }
    return NULL;
}

void pci_list_devices(void) {
    /* This will be called from shell to print device list */
    /* Shell will iterate through devices using pci_get_device() */
}

const char* pci_vendor_name(uint16_t vendor_id) {
    return pci_get_vendor_name(vendor_id);
}

const char* pci_class_name(uint8_t class_code) {
    return pci_get_class_name(class_code);
}