// kernel/fat.c (basic stub - reads boot sector and a fixed file)

#include "fat.h"
#include "disk.h"
#include "common.h"

struct fat_boot {
    uint8_t jump[3];
    char oem[8];
    uint16_t bytes_per_sector;
    // ... Add more fields as needed
} __attribute__((packed));

void fat_init() {
    uint8_t boot_sector[512];
    ata_read_sector(0, boot_sector);
    // Parse (stub)
}

void fat_load_file(const char *name, void *buf) {
    // Stub: Assume hello.txt at sector 1 (adjust for real FAT)
    ata_read_sector(1, (uint8_t*)buf);
}