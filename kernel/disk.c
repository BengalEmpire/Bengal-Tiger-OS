/**
 * Bengal Tiger OS - ATA Disk Driver Implementation
 * 
 * @file disk.c
 * @author Bengal Tiger OS (BengalEmpire)
 * @version 0.3.0
 */

#include "disk.h"
#include "common.h"

/* 400ns delay using I/O port read */
static void ata_delay(void) {
    for (int i = 0; i < 4; i++) {
        inb(ATA_STATUS);
    }
}

int ata_wait_ready(void) {
    uint8_t status;
    int timeout = 100000;
    
    /* Wait for BSY to clear */
    while (--timeout) {
        status = inb(ATA_STATUS);
        
        if (status & ATA_STATUS_ERR) {
            return -1;  /* Error */
        }
        
        if (!(status & ATA_STATUS_BSY) && (status & ATA_STATUS_DRDY)) {
            return 0;  /* Ready */
        }
    }
    
    return -1;  /* Timeout */
}

static int ata_wait_drq(void) {
    uint8_t status;
    int timeout = 100000;
    
    /* Wait for DRQ or error */
    while (--timeout) {
        status = inb(ATA_STATUS);
        
        if (status & ATA_STATUS_ERR) {
            return -1;
        }
        
        if (status & ATA_STATUS_DRQ) {
            return 0;
        }
    }
    
    return -1;
}

int ata_disk_present(void) {
    /* Check if primary master responds */
    outb(ATA_DRIVE_HEAD, 0xA0);  /* Select master */
    ata_delay();
    
    uint8_t status = inb(ATA_STATUS);
    
    /* If read returns 0xFF, no disk present */
    return (status != 0xFF && status != 0x00);
}

void ata_read_sector(uint32_t lba, uint8_t *buf) {
    /* Wait for drive to be ready */
    ata_wait_ready();
    
    /* Select drive (master) and set high LBA bits */
    /* 0xE0 = 1110 0000 = LBA mode, master drive, high 4 bits of LBA */
    outb(ATA_DRIVE_HEAD, 0xE0 | ((lba >> 24) & 0x0F));
    
    /* Set sector count */
    outb(ATA_SECTOR_CNT, 1);
    
    /* Set LBA address */
    outb(ATA_LBA_LOW,  lba & 0xFF);
    outb(ATA_LBA_MID,  (lba >> 8) & 0xFF);
    outb(ATA_LBA_HIGH, (lba >> 16) & 0xFF);
    
    /* Send read command */
    outb(ATA_COMMAND, ATA_CMD_READ);
    
    /* Wait for data */
    if (ata_wait_drq() != 0) {
        /* Error - fill buffer with zeros */
        memset(buf, 0, SECTOR_SIZE);
        return;
    }
    
    /* Read 256 words (512 bytes) */
    uint16_t *buf16 = (uint16_t*)buf;
    for (int i = 0; i < 256; i++) {
        buf16[i] = inw(ATA_DATA);
    }
}

void ata_write_sector(uint32_t lba, uint8_t *buf) {
    /* Wait for drive to be ready */
    ata_wait_ready();
    
    /* Select drive and set high LBA bits */
    outb(ATA_DRIVE_HEAD, 0xE0 | ((lba >> 24) & 0x0F));
    
    /* Set sector count */
    outb(ATA_SECTOR_CNT, 1);
    
    /* Set LBA address */
    outb(ATA_LBA_LOW,  lba & 0xFF);
    outb(ATA_LBA_MID,  (lba >> 8) & 0xFF);
    outb(ATA_LBA_HIGH, (lba >> 16) & 0xFF);
    
    /* Send write command */
    outb(ATA_COMMAND, ATA_CMD_WRITE);
    
    /* Wait for drive to request data */
    if (ata_wait_drq() != 0) {
        return;  /* Error */
    }
    
    /* Write 256 words */
    uint16_t *buf16 = (uint16_t*)buf;
    for (int i = 0; i < 256; i++) {
        outw(ATA_DATA, buf16[i]);
    }
    
    /* Flush cache */
    outb(ATA_COMMAND, 0xE7);  /* CACHE FLUSH */
    ata_wait_ready();
}

void ata_read_sectors(uint32_t lba, uint32_t count, uint8_t *buf) {
    for (uint32_t i = 0; i < count; i++) {
        ata_read_sector(lba + i, buf + (i * SECTOR_SIZE));
    }
}