/**
 * Bengal Tiger OS - Enhanced ATA/ATAPI Disk Driver Implementation
 *
 * @file disk.c
 * @author Bengal Tiger OS (BengalEmpire)
 * @version 0.4.0
 */

#include "disk.h"
#include "common.h"

/* Primary master drive info */
static ata_drive_t primary_master;

static uint16_t ata_base_port(uint8_t channel) {
    return (channel == 0) ? ATA_DATA : ATA2_DATA;
}

static void ata_delay(void) {
    for (int i = 0; i < 4; i++) {
        inb(ATA_ALT_STATUS);
    }
}

int ata_wait_ready(void) {
    uint8_t status;
    int timeout = ATA_TIMEOUT;
    while (--timeout) {
        status = inb(ATA_STATUS);
        if (status & ATA_STATUS_ERR) return -1;
        if (!(status & ATA_STATUS_BSY)) {
            if (status & ATA_STATUS_DRDY) return 0;
            if (timeout < ATA_TIMEOUT / 10) return 0;
        }
    }
    return -1;
}

static int ata_wait_drq(void) {
    uint8_t status;
    int timeout = ATA_TIMEOUT;
    while (--timeout) {
        status = inb(ATA_STATUS);
        if (status & ATA_STATUS_ERR) return -1;
        if (status & ATA_STATUS_DRQ) return 0;
        if (!(status & ATA_STATUS_BSY) && !(status & ATA_STATUS_DRQ)) return -1;
    }
    return -1;
}

void ata_soft_reset(uint8_t channel) {
    uint16_t ctrl = (channel == 0) ? ATA_CTRL : ATA2_DATA;
    outb(ctrl, 0x04);
    ata_delay();
    outb(ctrl, 0x00);
    ata_delay();
}

int ata_identify(ata_drive_t *drive, uint8_t channel, uint8_t is_master) {
    if (!drive) return -1;
    uint16_t base = ata_base_port(channel);
    uint16_t dev_head = is_master ? ATA_DRIVE_MASTER : ATA_DRIVE_SLAVE;

    outb(base + 6, dev_head);
    ata_delay();
    outb(base + 2, 0);
    outb(base + 3, 0);
    outb(base + 4, 0);
    outb(base + 5, 0);
    outb(base + 7, ATA_CMD_IDENTIFY);

    uint8_t status = inb(base + 7);
    if (status == 0) return -1;

    int timeout = ATA_TIMEOUT;
    while (timeout--) {
        status = inb(base + 7);
        if (!(status & ATA_STATUS_BSY)) break;
    }
    if (timeout <= 0) return -1;

    if (status & ATA_STATUS_ERR) {
        uint8_t err = inb(base + 1);
        if (err & 0x04) {
            outb(base + 7, ATA_CMD_IDENTIFY_PACKET);
            timeout = ATA_TIMEOUT;
            while (timeout--) {
                status = inb(base + 7);
                if (!(status & ATA_STATUS_BSY)) break;
            }
            if (timeout <= 0) return -1;
            if (status & ATA_STATUS_ERR) return -1;
            drive->drive_type = ATA_TYPE_ATAPI;
        } else {
            return -1;
        }
    } else {
        drive->drive_type = ATA_TYPE_ATA;
    }

    if (!(status & ATA_STATUS_DRQ)) {
        timeout = ATA_TIMEOUT;
        while (timeout--) {
            status = inb(base + 7);
            if (status & ATA_STATUS_DRQ) break;
            if (status & ATA_STATUS_ERR) return -1;
        }
        if (timeout <= 0) return -1;
    }

    uint16_t identify[256];
    for (int i = 0; i < 256; i++) {
        identify[i] = inw(base);
    }

    drive->channel = channel;
    drive->is_master = is_master;
    drive->present = 1;

    for (int i = 0; i < 40; i += 2) {
        drive->model[i]     = (identify[27 + i/2] >> 8) & 0xFF;
        drive->model[i + 1] =  identify[27 + i/2] & 0xFF;
    }
    drive->model[40] = '\0';
    char *p = drive->model;
    while (*p == ' ') p++;
    if (p != drive->model) memcpy(drive->model, p, 41 - (p - drive->model));
    int len = strlen(drive->model);
    while (len > 0 && drive->model[len - 1] == ' ') drive->model[--len] = '\0';

    for (int i = 0; i < 20; i += 2) {
        drive->serial[i]     = (identify[10 + i/2] >> 8) & 0xFF;
        drive->serial[i + 1] =  identify[10 + i/2] & 0xFF;
    }
    drive->serial[20] = '\0';

    for (int i = 0; i < 8; i += 2) {
        drive->firmware[i]     = (identify[23 + i/2] >> 8) & 0xFF;
        drive->firmware[i + 1] =  identify[23 + i/2] & 0xFF;
    }
    drive->firmware[8] = '\0';

    drive->is_lba48 = (identify[83] & (1 << 10)) ? 1 : 0;
    drive->sectors_28 = *(uint32_t*)&identify[60];
    if (drive->is_lba48) {
        drive->sectors_48 = *(uint64_t*)&identify[100];
    } else {
        drive->sectors_48 = drive->sectors_28;
    }
    drive->pio_mode = (identify[64] >> 8) & 0xFF;
    drive->dma_mode = identify[63] & 0xFF;
    return 0;
}

void disk_init(void) {
    memset(&primary_master, 0, sizeof(primary_master));
    ata_identify(&primary_master, 0, 1);
}

int ata_disk_present(void) {
    if (primary_master.present) return 1;
    outb(ATA_DRIVE_HEAD, ATA_DRIVE_MASTER);
    ata_delay();
    uint8_t status = inb(ATA_STATUS);
    return (status != 0xFF && status != 0x00);
}

static int ata_select_and_setup(uint32_t lba, uint8_t master) {
    if (ata_wait_ready() != 0) return -1;
    outb(ATA_DRIVE_HEAD, (master ? ATA_DRIVE_MASTER : ATA_DRIVE_SLAVE) |
         ATA_DRIVE_LBA | ((lba >> 24) & 0x0F));
    outb(ATA_SECTOR_CNT, 1);
    outb(ATA_LBA_LOW,  lba & 0xFF);
    outb(ATA_LBA_MID,  (lba >> 8) & 0xFF);
    outb(ATA_LBA_HIGH, (lba >> 16) & 0xFF);
    return 0;
}

int ata_read_sector(uint32_t lba, uint8_t *buf) {
    int retries = ATA_MAX_RETRIES;
    while (retries-- > 0) {
        if (ata_select_and_setup(lba, 1) != 0) continue;
        outb(ATA_COMMAND, ATA_CMD_READ_PIO);
        if (ata_wait_drq() != 0) { ata_soft_reset(0); continue; }
        uint16_t *buf16 = (uint16_t*)buf;
        for (int i = 0; i < 256; i++) buf16[i] = inw(ATA_DATA);
        return 0;
    }
    memset(buf, 0, SECTOR_SIZE);
    return -1;
}

int ata_write_sector(uint32_t lba, uint8_t *buf) {
    int retries = ATA_MAX_RETRIES;
    while (retries-- > 0) {
        if (ata_select_and_setup(lba, 1) != 0) continue;
        outb(ATA_COMMAND, ATA_CMD_WRITE_PIO);
        if (ata_wait_drq() != 0) { ata_soft_reset(0); continue; }
        uint16_t *buf16 = (uint16_t*)buf;
        for (int i = 0; i < 256; i++) outw(ATA_DATA, buf16[i]);
        outb(ATA_COMMAND, ATA_CMD_CACHE_FLUSH);
        ata_wait_ready();
        return 0;
    }
    return -1;
}

int ata_read_sectors(uint32_t lba, uint32_t count, uint8_t *buf) {
    for (uint32_t i = 0; i < count; i++) {
        if (ata_read_sector(lba + i, buf + (i * SECTOR_SIZE)) != 0) return -1;
    }
    return 0;
}

ata_drive_t* ata_get_drive_info(void) {
    return primary_master.present ? &primary_master : NULL;
}

uint64_t ata_get_capacity(void) {
    return primary_master.present ? primary_master.sectors_48 * SECTOR_SIZE : 0;
}