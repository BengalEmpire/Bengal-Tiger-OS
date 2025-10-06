// kernel/disk.c

#include "disk.h"

void ata_read_sector(uint32_t lba, uint8_t *buf) {
    outb(0x1F6, (lba >> 24) | 0xE0);
    outb(0x1F2, 1);
    outb(0x1F3, lba);
    outb(0x1F4, lba >> 8);
    outb(0x1F5, lba >> 16);
    outb(0x1F7, 0x20);
    while (!(inb(0x1F7) & 0x08));
    for (int i = 0; i < 256; i++) {
        ((uint16_t*)buf)[i] = inw(0x1F0);
    }
}