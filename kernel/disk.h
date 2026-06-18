/**
 * Bengal Tiger OS - Enhanced ATA/ATAPI Disk Driver
 *
 * Full PIO-mode ATA/ATAPI disk driver with:
 *   - IDENTIFY command for drive detection
 *   - ATAPI (CD-ROM) detection
 *   - Error recovery with retries
 *   - 28-bit and 48-bit LBA support
 *   - Drive capacity reporting
 *
 * @file disk.h
 * @author Bengal Tiger OS (BengalEmpire)
 * @version 0.4.0
 */

#ifndef DISK_H
#define DISK_H

#include "common.h"

/* ============================================ */
/* ATA I/O Ports (Primary Channel)              */
/* ============================================ */

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

/* Alternate Status / Control port */
#define ATA_ALT_STATUS  0x3F6
#define ATA_CTRL        0x3F6

/* Secondary channel ports */
#define ATA2_DATA       0x170
#define ATA2_ERROR      0x171
#define ATA2_SECTOR_CNT 0x172
#define ATA2_LBA_LOW    0x173
#define ATA2_LBA_MID    0x174
#define ATA2_LBA_HIGH   0x175
#define ATA2_DRIVE_HEAD 0x176
#define ATA2_STATUS     0x177
#define ATA2_COMMAND    0x177

/* ============================================ */
/* ATA Commands                                 */
/* ============================================ */

#define ATA_CMD_READ_PIO      0x20   /* Read sectors (PIO) */
#define ATA_CMD_READ_PIO_EXT  0x24   /* Read sectors (PIO, 48-bit LBA) */
#define ATA_CMD_WRITE_PIO     0x30   /* Write sectors (PIO) */
#define ATA_CMD_WRITE_PIO_EXT 0x34   /* Write sectors (PIO, 48-bit LBA) */
#define ATA_CMD_IDENTIFY      0xEC   /* Identify drive */
#define ATA_CMD_IDENTIFY_PACKET 0xA1 /* Identify packet device (ATAPI) */
#define ATA_CMD_PACKET        0xA0   /* Packet command (ATAPI) */
#define ATA_CMD_CACHE_FLUSH   0xE7   /* Cache flush */
#define ATA_CMD_SET_FEATURES  0xEF   /* Set features */

/* ============================================ */
/* ATA Status Bits                              */
/* ============================================ */

#define ATA_STATUS_BSY   0x80    /* Drive busy */
#define ATA_STATUS_DRDY  0x40    /* Drive ready */
#define ATA_STATUS_DF    0x20    /* Drive fault */
#define ATA_STATUS_DSC   0x10    /* Drive seek complete */
#define ATA_STATUS_DRQ   0x08    /* Data request ready */
#define ATA_STATUS_CORR  0x04    /* Corrected data */
#define ATA_STATUS_IDX   0x02    /* Index */
#define ATA_STATUS_ERR   0x01    /* Error occurred */

/* ============================================ */
/* Drive Head Register Bits                     */
/* ============================================ */

#define ATA_DRIVE_MASTER    0xA0
#define ATA_DRIVE_SLAVE     0xB0
#define ATA_DRIVE_LBA       (1 << 6)  /* LBA mode bit */

/* ============================================ */
/* Drive Types                                  */
/* ============================================ */

#define ATA_TYPE_NONE       0
#define ATA_TYPE_ATA        1   /* Hard drive */
#define ATA_TYPE_ATAPI      2   /* CD/DVD drive */

/* ============================================ */
/* Constants                                    */
/* ============================================ */

#define SECTOR_SIZE         512
#define ATA_MAX_RETRIES     3
#define ATA_TIMEOUT         1000000

/* ============================================ */
/* ATA Drive Information Structure              */
/* ============================================ */

typedef struct {
    uint8_t  drive_type;         /* ATA_TYPE_* */
    uint8_t  channel;            /* 0 = primary, 1 = secondary */
    uint8_t  is_master;          /* 1 = master, 0 = slave */
    uint8_t  is_lba48;           /* Supports 48-bit LBA */
    uint32_t sectors_28;         /* Sectors if using 28-bit LBA */
    uint64_t sectors_48;         /* Sectors if using 48-bit LBA */
    char     model[41];          /* Model string */
    char     serial[21];         /* Serial number */
    char     firmware[9];        /* Firmware revision */
    uint16_t pio_mode;           /* PIO mode supported */
    uint16_t dma_mode;           /* DMA mode supported */
    uint8_t  present;            /* 1 if drive present */
} ata_drive_t;

/* ============================================ */
/* Function Prototypes                          */
/* ============================================ */

/**
 * Initialize ATA driver.
 * Detects and identifies all drives on primary and secondary channels.
 */
void disk_init(void);

/**
 * Wait for ATA drive to be ready.
 * @return 0 on success, -1 on timeout/error
 */
int ata_wait_ready(void);

/**
 * Check if primary master ATA disk is present.
 * @return 1 if present, 0 if not
 */
int ata_disk_present(void);

/**
 * Read a sector from disk.
 * @param lba Logical Block Address
 * @param buf Buffer (must be SECTOR_SIZE bytes)
 * @return 0 on success, -1 on error
 */
int ata_read_sector(uint32_t lba, uint8_t *buf);

/**
 * Write a sector to disk.
 * @param lba Logical Block Address
 * @param buf Buffer (SECTOR_SIZE bytes)
 * @return 0 on success, -1 on error
 */
int ata_write_sector(uint32_t lba, uint8_t *buf);

/**
 * Read multiple sectors.
 * @param lba Starting LBA
 * @param count Number of sectors
 * @param buf Buffer (count * SECTOR_SIZE bytes)
 * @return 0 on success, -1 on error
 */
int ata_read_sectors(uint32_t lba, uint32_t count, uint8_t *buf);

/**
 * Execute IDENTIFY command on a drive.
 * Populates the ata_drive_t structure.
 * @param drive Pointer to drive structure
 * @param channel 0 = primary, 1 = secondary
 * @param is_master 1 = master, 0 = slave
 * @return 0 on success, -1 on error
 */
int ata_identify(ata_drive_t *drive, uint8_t channel, uint8_t is_master);

/**
 * Get pointer to identified drive information.
 * @return Pointer to ata_drive_t structure
 */
ata_drive_t* ata_get_drive_info(void);

/**
 * Get drive capacity in bytes (for shell display).
 * @return Capacity in bytes
 */
uint64_t ata_get_capacity(void);

/**
 * Soft reset an ATA channel.
 * @param channel 0 = primary, 1 = secondary
 */
void ata_soft_reset(uint8_t channel);

#endif /* DISK_H */