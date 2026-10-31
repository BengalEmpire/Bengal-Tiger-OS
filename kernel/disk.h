/**
 * Bengal Tiger OS - Enhanced ATA/ATAPI & Live RAMDisk Block Driver
 *
 * Scans Primary & Secondary IDE channels (Master & Slave), identifies
 * ATA hard disks and ATAPI optical drives, supports 28-bit PIO read/write
 * with retry & soft-reset recovery, and provides an automatic 512KB Live
 * RAMDisk fallback when booting from ISO/USB without a formatted HDD.
 *
 * @file disk.h
 * @author Bengal Tiger OS (BengalEmpire)
 * @version 1.0.0
 */

#ifndef DISK_H
#define DISK_H

#include "common.h"

/* Primary ATA Channel Ports */
#define ATA_DATA            0x1F0
#define ATA_ERROR           0x1F1
#define ATA_FEATURES        0x1F1
#define ATA_SECTOR_CNT      0x1F2
#define ATA_LBA_LOW         0x1F3
#define ATA_LBA_MID         0x1F4
#define ATA_LBA_HIGH        0x1F5
#define ATA_DRIVE_HEAD      0x1F6
#define ATA_STATUS          0x1F7
#define ATA_COMMAND         0x1F7
#define ATA_ALT_STATUS      0x3F6
#define ATA_CTRL            0x3F6

/* Secondary ATA Channel Ports */
#define ATA2_DATA           0x170
#define ATA2_ERROR          0x171
#define ATA2_SECTOR_CNT     0x172
#define ATA2_LBA_LOW        0x173
#define ATA2_LBA_MID        0x174
#define ATA2_LBA_HIGH       0x175
#define ATA2_DRIVE_HEAD     0x176
#define ATA2_STATUS         0x177
#define ATA2_COMMAND        0x177
#define ATA2_ALT_STATUS     0x376
#define ATA2_CTRL           0x376

/* ATA Commands */
#define ATA_CMD_READ_PIO        0x20
#define ATA_CMD_READ_PIO_EXT    0x24
#define ATA_CMD_WRITE_PIO       0x30
#define ATA_CMD_WRITE_PIO_EXT   0x34
#define ATA_CMD_IDENTIFY        0xEC
#define ATA_CMD_IDENTIFY_PACKET 0xA1
#define ATA_CMD_PACKET          0xA0
#define ATA_CMD_CACHE_FLUSH     0xE7

/* ATA Status Bits */
#define ATA_STATUS_BSY      0x80
#define ATA_STATUS_DRDY     0x40
#define ATA_STATUS_DF       0x20
#define ATA_STATUS_DSC      0x10
#define ATA_STATUS_DRQ      0x08
#define ATA_STATUS_ERR      0x01

/* Drive Select Bits */
#define ATA_DRIVE_MASTER    0xA0
#define ATA_DRIVE_SLAVE     0xB0
#define ATA_DRIVE_LBA       (1 << 6)

/* Drive Types */
#define ATA_TYPE_NONE       0
#define ATA_TYPE_ATA        1
#define ATA_TYPE_ATAPI      2

#define SECTOR_SIZE         512
#define ATA_MAX_RETRIES     3
#define ATA_TIMEOUT         200000
#define RAMDISK_SECTORS     1024    /* 512 KB Live RAMDisk */

typedef struct {
    uint8_t  drive_type;
    uint8_t  channel;
    uint8_t  is_master;
    uint8_t  is_lba48;
    uint32_t sectors_28;
    uint64_t sectors_48;
    char     model[41];
    char     serial[21];
    char     firmware[9];
    uint16_t pio_mode;
    uint16_t dma_mode;
    uint8_t  present;
} ata_drive_t;

void         disk_init(void);
int          ata_wait_ready(void);
int          ata_disk_present(void);
int          disk_has_hardware_hdd(void);
int          disk_enable_ramdisk(int enable);
int          disk_is_ramdisk(void);
uint32_t     disk_get_total_sectors(void);

int          ata_read_sector(uint32_t lba, uint8_t *buf);
int          ata_write_sector(uint32_t lba, const uint8_t *buf);
int          ata_read_sectors(uint32_t lba, uint32_t count, uint8_t *buf);
int          ata_write_sectors(uint32_t lba, uint32_t count, const uint8_t *buf);

int          ata_identify(ata_drive_t *drive, uint8_t channel, uint8_t is_master);
ata_drive_t* ata_get_drive_info(void);
int          ata_get_drive_count(void);
ata_drive_t* ata_get_drive(int index);
uint64_t     ata_get_capacity(void);
void         ata_soft_reset(uint8_t channel);

#endif /* DISK_H */
