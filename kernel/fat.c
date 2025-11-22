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
    // Enhanced stub: Check name and load from fixed sectors
    if (strcmp(name, "hello.txt") == 0) {
        ata_read_sector(1, (uint8_t*)buf);
    } else if (strcmp(name, "config.cfg") == 0) {
        ata_read_sector(2, (uint8_t*)buf);
    }
}

void fat_save_file(const char *name, void *buf) {
    // Stub write
    if (strcmp(name, "config.cfg") == 0) {
        ata_write_sector(2, (uint8_t*)buf);
    }
}