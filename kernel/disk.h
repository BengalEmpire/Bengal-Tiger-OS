/**
 * Bengal Tiger OS - ATA Disk Driver
 * 
 * Simple PIO-mode ATA disk driver for reading and writing sectors.
 * 
 * @file disk.h
 * @author Bengal Tiger OS (BengalEmpire)
 * @version 0.3.0
 */

#ifndef DISK_H
#define DISK_H

#include "common.h"

/* ATA I/O Ports (Primary Channel) */
#define ATA_DATA        0x1F0
#define ATA_ERROR       0x1F1
#define ATA_FEATURES    0x1F1
#define ATA_SECTOR_CNT  0x1F2
#define ATA_LBA_LOW     0x1F3
#define ATA_LBA_MID     0x1F4
#define ATA_LBA_HIGH    0x1F5
#define ATA_DRIVE_HEAD  0x1F6
#define ATA_STATUS      0x1F7
#define ATA_COMMAND     0x1F7

/* ATA Commands */
#define ATA_CMD_READ    0x20
#define ATA_CMD_WRITE   0x30
#define ATA_CMD_IDENTIFY 0xEC

/* ATA Status Bits */
#define ATA_STATUS_BSY  0x80    /* Drive busy */
#define ATA_STATUS_DRDY 0x40    /* Drive ready */
#define ATA_STATUS_DRQ  0x08    /* Data request ready */
#define ATA_STATUS_ERR  0x01    /* Error occurred */

/* Sector size */
#define SECTOR_SIZE 512

/**
 * Read a sector from disk.
 * 
 * @param lba Logical Block Address (sector number)
 * @param buf Buffer to read into (must be at least 512 bytes)
 */
void ata_read_sector(uint32_t lba, uint8_t *buf);

/**
 * Write a sector to disk.
 * 
 * @param lba Logical Block Address
 * @param buf Buffer to write from (512 bytes)
 */
void ata_write_sector(uint32_t lba, uint8_t *buf);

/**
 * Read multiple sectors.
 * 
 * @param lba Starting LBA
 * @param count Number of sectors to read
 * @param buf Buffer (must be count * 512 bytes)
 */
void ata_read_sectors(uint32_t lba, uint32_t count, uint8_t *buf);

/**
 * Wait for disk to be ready.
 * 
 * @return 0 on success, -1 on error
 */
int ata_wait_ready(void);

/**
 * Check if primary ATA disk is present.
 * 
 * @return 1 if present, 0 if not
 */
int ata_disk_present(void);

#endif /* DISK_H */