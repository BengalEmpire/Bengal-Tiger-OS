/**
 * Bengal Tiger OS - Enhanced ATA/ATAPI & Live RAMDisk Implementation
 *
 * @file disk.c
 * @author Bengal Tiger OS (BengalEmpire)
 * @version 1.0.0
 */

#include "disk.h"
#include "heap.h"
#include "common.h"

static ata_drive_t detected_drives[4];
static int detected_count = 0;
static ata_drive_t *active_hdd = NULL;

/* Live RAMDisk state */
static uint8_t *ramdisk_buf = NULL;
static int ramdisk_active   = 0;

static uint16_t ata_base_port(uint8_t channel) {
    return (channel == 0) ? ATA_DATA : ATA2_DATA;
}

static uint16_t ata_ctrl_port(uint8_t channel) {
    return (channel == 0) ? ATA_CTRL : ATA2_CTRL;
}

static void ata_delay_ch(uint8_t channel) {
    uint16_t ctrl = ata_ctrl_port(channel);
    for (int i = 0; i < 4; i++) {
        (void)inb(ctrl);
    }
}

static int ata_wait_not_bsy(uint16_t base) {
    int timeout = ATA_TIMEOUT;
    while (--timeout > 0) {
        uint8_t status = inb(base + 7);
        if (status == 0xFF) return -1;
        if (!(status & ATA_STATUS_BSY)) return 0;
    }
    return -1;
}

int ata_wait_ready(void) {
    uint8_t ch = active_hdd ? active_hdd->channel : 0;
    uint16_t base = ata_base_port(ch);
    int timeout = ATA_TIMEOUT;
    while (--timeout > 0) {
        uint8_t status = inb(base + 7);
        if (status == 0xFF) return -1;
        if (status & (ATA_STATUS_ERR | ATA_STATUS_DF)) return -1;
        if (!(status & ATA_STATUS_BSY) && (status & ATA_STATUS_DRDY)) {
            return 0;
        }
    }
    return -1;
}

static int ata_wait_drq_ch(uint16_t base) {
    int timeout = ATA_TIMEOUT;
    while (--timeout > 0) {
        uint8_t status = inb(base + 7);
        if (status & (ATA_STATUS_ERR | ATA_STATUS_DF)) return -1;
        if (!(status & ATA_STATUS_BSY) && (status & ATA_STATUS_DRQ)) return 0;
    }
    return -1;
}

void ata_soft_reset(uint8_t channel) {
    uint16_t ctrl = ata_ctrl_port(channel);
    outb(ctrl, 0x04);
    ata_delay_ch(channel);
    outb(ctrl, 0x00);
    ata_delay_ch(channel);
}

static void trim_ata_string(char *str, int max_len) {
    str[max_len] = '\0';
    int len = strlen(str);
    while (len > 0 && str[len - 1] == ' ') {
        str[--len] = '\0';
    }
    char *p = str;
    while (*p == ' ') p++;
    if (p != str) {
        int rem = strlen(p);
        for (int i = 0; i <= rem; i++) str[i] = p[i];
    }
}

int ata_identify(ata_drive_t *drive, uint8_t channel, uint8_t is_master) {
    if (!drive) return -1;
    memset(drive, 0, sizeof(ata_drive_t));

    uint16_t base = ata_base_port(channel);
    /* Floating bus check */
    if (inb(base + 7) == 0xFF) return -1;

    uint8_t dev_sel = is_master ? ATA_DRIVE_MASTER : ATA_DRIVE_SLAVE;
    outb(base + 6, dev_sel);
    ata_delay_ch(channel);

    outb(base + 2, 0);
    outb(base + 3, 0);
    outb(base + 4, 0);
    outb(base + 5, 0);
    outb(base + 7, ATA_CMD_IDENTIFY);
    ata_delay_ch(channel);

    uint8_t status = inb(base + 7);
    if (status == 0 || status == 0xFF) return -1;

    if (ata_wait_not_bsy(base) != 0) return -1;

    uint8_t cl = inb(base + 4);
    uint8_t ch_hi = inb(base + 5);
    status = inb(base + 7);

    if ((cl == 0x14 && ch_hi == 0xEB) || (cl == 0x69 && ch_hi == 0x96) || (status & ATA_STATUS_ERR)) {
        /* Try ATAPI IDENTIFY PACKET DEVICE */
        outb(base + 7, ATA_CMD_IDENTIFY_PACKET);
        ata_delay_ch(channel);
        if (ata_wait_not_bsy(base) != 0) return -1;
        status = inb(base + 7);
        if (status & ATA_STATUS_ERR) return -1;
        drive->drive_type = ATA_TYPE_ATAPI;
    } else if (cl == 0 && ch_hi == 0) {
        drive->drive_type = ATA_TYPE_ATA;
    } else {
        return -1;
    }

    if (ata_wait_drq_ch(base) != 0) return -1;

    uint16_t identify[256];
    for (int i = 0; i < 256; i++) {
        identify[i] = inw(base);
    }

    drive->channel   = channel;
    drive->is_master = is_master;
    drive->present   = 1;

    for (int i = 0; i < 40; i += 2) {
        drive->model[i]     = (char)((identify[27 + i / 2] >> 8) & 0xFF);
        drive->model[i + 1] = (char)( identify[27 + i / 2]       & 0xFF);
    }
    trim_ata_string(drive->model, 40);

    for (int i = 0; i < 20; i += 2) {
        drive->serial[i]     = (char)((identify[10 + i / 2] >> 8) & 0xFF);
        drive->serial[i + 1] = (char)( identify[10 + i / 2]       & 0xFF);
    }
    trim_ata_string(drive->serial, 20);

    for (int i = 0; i < 8; i += 2) {
        drive->firmware[i]     = (char)((identify[23 + i / 2] >> 8) & 0xFF);
        drive->firmware[i + 1] = (char)( identify[23 + i / 2]       & 0xFF);
    }
    trim_ata_string(drive->firmware, 8);

    drive->is_lba48 = (identify[83] & (1 << 10)) ? 1 : 0;

    uint32_t sectors_28 = 0;
    memcpy(&sectors_28, &identify[60], sizeof(uint32_t));
    drive->sectors_28 = sectors_28;

    if (drive->is_lba48) {
        uint64_t sectors_48 = 0;
        memcpy(&sectors_48, &identify[100], sizeof(uint64_t));
        drive->sectors_48 = sectors_48 ? sectors_48 : sectors_28;
    } else {
        drive->sectors_48 = sectors_28;
    }

    drive->pio_mode = (identify[64] >> 8) & 0xFF;
    drive->dma_mode = identify[63] & 0xFF;
    return 0;
}

void disk_init(void) {
    memset(detected_drives, 0, sizeof(detected_drives));
    detected_count = 0;
    active_hdd = NULL;
    ramdisk_active = 0;

    for (uint8_t ch = 0; ch < 2; ch++) {
        for (int m = 1; m >= 0; m--) {
            ata_drive_t tmp;
            if (ata_identify(&tmp, ch, (uint8_t)m) == 0 && tmp.present) {
                if (detected_count < 4) {
                    detected_drives[detected_count] = tmp;
                    if (!active_hdd &&
                        tmp.drive_type == ATA_TYPE_ATA &&
                        (tmp.sectors_28 > 0 || tmp.sectors_48 > 0)) {
                        active_hdd = &detected_drives[detected_count];
                    }
                    detected_count++;
                }
            }
        }
    }
}

int disk_has_hardware_hdd(void) {
    return (active_hdd != NULL && active_hdd->present) ? 1 : 0;
}

int disk_enable_ramdisk(int enable) {
    if (enable) {
        if (!ramdisk_buf) {
            ramdisk_buf = (uint8_t*)kzalloc(RAMDISK_SECTORS * SECTOR_SIZE);
            if (!ramdisk_buf) return -1;
        }
        ramdisk_active = 1;
        return 0;
    } else {
        if (!disk_has_hardware_hdd()) return -1;
        ramdisk_active = 0;
        return 0;
    }
}

int disk_is_ramdisk(void) {
    return ramdisk_active;
}

uint32_t disk_get_total_sectors(void) {
    if (ramdisk_active) {
        return RAMDISK_SECTORS;
    }
    if (active_hdd && active_hdd->present) {
        if (active_hdd->sectors_48 > 0x0FFFFFFFULL) {
            return 0x0FFFFFFFU; /* LBA28 PIO limit (128GB) */
        }
        return (uint32_t)active_hdd->sectors_48;
    }
    return 0;
}

int ata_disk_present(void) {
    return (disk_has_hardware_hdd() || ramdisk_active) ? 1 : 0;
}

static int ata_setup_lba28(ata_drive_t *drv, uint32_t lba) {
    uint16_t base = ata_base_port(drv->channel);
    if (ata_wait_not_bsy(base) != 0) return -1;

    uint8_t sel = (drv->is_master ? ATA_DRIVE_MASTER : ATA_DRIVE_SLAVE) |
                  ATA_DRIVE_LBA | (uint8_t)((lba >> 24) & 0x0F);
    outb(base + 6, sel);
    ata_delay_ch(drv->channel);

    if (ata_wait_ready() != 0) return -1;

    outb(base + 2, 1);
    outb(base + 3, (uint8_t)(lba & 0xFF));
    outb(base + 4, (uint8_t)((lba >> 8) & 0xFF));
    outb(base + 5, (uint8_t)((lba >> 16) & 0xFF));
    return 0;
}

int ata_read_sector(uint32_t lba, uint8_t *buf) {
    if (!buf) return -1;

    if (ramdisk_active) {
        if (!ramdisk_buf || lba >= RAMDISK_SECTORS) {
            memset(buf, 0, SECTOR_SIZE);
            return -1;
        }
        memcpy(buf, ramdisk_buf + lba * SECTOR_SIZE, SECTOR_SIZE);
        return 0;
    }

    if (!active_hdd || !active_hdd->present) {
        memset(buf, 0, SECTOR_SIZE);
        return -1;
    }

    uint16_t base = ata_base_port(active_hdd->channel);
    int retries = ATA_MAX_RETRIES;
    while (retries-- > 0) {
        if (ata_setup_lba28(active_hdd, lba) != 0) {
            ata_soft_reset(active_hdd->channel);
            continue;
        }
        outb(base + 7, ATA_CMD_READ_PIO);
        if (ata_wait_drq_ch(base) != 0) {
            ata_soft_reset(active_hdd->channel);
            continue;
        }
        uint16_t *buf16 = (uint16_t*)buf;
        for (int i = 0; i < 256; i++) {
            buf16[i] = inw(base);
        }
        return 0;
    }

    memset(buf, 0, SECTOR_SIZE);
    return -1;
}

int ata_write_sector(uint32_t lba, const uint8_t *buf) {
    if (!buf) return -1;

    if (ramdisk_active) {
        if (!ramdisk_buf || lba >= RAMDISK_SECTORS) return -1;
        memcpy(ramdisk_buf + lba * SECTOR_SIZE, buf, SECTOR_SIZE);
        return 0;
    }

    if (!active_hdd || !active_hdd->present) {
        return -1;
    }

    uint16_t base = ata_base_port(active_hdd->channel);
    int retries = ATA_MAX_RETRIES;
    while (retries-- > 0) {
        if (ata_setup_lba28(active_hdd, lba) != 0) {
            ata_soft_reset(active_hdd->channel);
            continue;
        }
        outb(base + 7, ATA_CMD_WRITE_PIO);
        if (ata_wait_drq_ch(base) != 0) {
            ata_soft_reset(active_hdd->channel);
            continue;
        }
        const uint16_t *buf16 = (const uint16_t*)buf;
        for (int i = 0; i < 256; i++) {
            outw(base, buf16[i]);
        }
        outb(base + 7, ATA_CMD_CACHE_FLUSH);
        ata_wait_not_bsy(base);
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

int ata_write_sectors(uint32_t lba, uint32_t count, const uint8_t *buf) {
    for (uint32_t i = 0; i < count; i++) {
        if (ata_write_sector(lba + i, buf + (i * SECTOR_SIZE)) != 0) return -1;
    }
    return 0;
}

ata_drive_t* ata_get_drive_info(void) {
    if (active_hdd) return active_hdd;
    return (detected_count > 0) ? &detected_drives[0] : NULL;
}

int ata_get_drive_count(void) {
    return detected_count;
}

ata_drive_t* ata_get_drive(int index) {
    if (index < 0 || index >= detected_count) return NULL;
    return &detected_drives[index];
}

uint64_t ata_get_capacity(void) {
    if (active_hdd && active_hdd->present) {
        return active_hdd->sectors_48 * (uint64_t)SECTOR_SIZE;
    }
    if (ramdisk_active) {
        return (uint64_t)RAMDISK_SECTORS * SECTOR_SIZE;
    }
    return 0;
}
