/**
 * Bengal Tiger OS - FAT Filesystem Driver
 *
 * Supports FAT12 and FAT16 reading.
 * 
 * @file fat.c
 * @author Bengal Tiger OS (BengalEmpire)
 * @version 0.6.0
 */

#include "fat.h"
#include "disk.h"
#include "common.h"
#include "heap.h"
#include "shell.h"

/* FAT Boot Sector Structure */
typedef struct {
    uint8_t  jump[3];
    char     oem[8];
    uint16_t bytes_per_sector;
    uint8_t  sectors_per_cluster;
    uint16_t reserved_sectors;
    uint8_t  num_fats;
    uint16_t root_entries;
    uint16_t total_sectors_16;
    uint8_t  media_type;
    uint16_t fat_size_16;
    uint16_t sectors_per_track;
    uint16_t num_heads;
    uint32_t hidden_sectors;
    uint32_t total_sectors_32;

    /* Extended BIOS Parameter Block */
    uint8_t  drive_num;
    uint8_t  reserved1;
    uint8_t  boot_sig;
    uint32_t vol_id;
    char     vol_label[11];
    char     fs_type[8];
} __attribute__((packed)) fat_boot_t;

/* FAT Directory Entry */
typedef struct {
    char     name[8];
    char     ext[3];
    uint8_t  attr;
    uint8_t  reserved;
    uint8_t  ctime_ms;
    uint16_t ctime;
    uint16_t cdate;
    uint16_t adate;
    uint16_t cluster_high;
    uint16_t mtime;
    uint16_t mdate;
    uint16_t cluster_low;
    uint32_t size;
} __attribute__((packed)) fat_dirent_t;

static fat_boot_t boot_sector;
static uint32_t fat_start_sector;
static uint32_t root_start_sector;
static uint32_t root_sectors;
static uint32_t data_start_sector;
static int fat_type = 0; /* 12 or 16 */

void fat_init(void) {
    uint8_t buf[512];
    if (ata_read_sector(0, buf) != 0) return;

    memcpy(&boot_sector, buf, sizeof(fat_boot_t));

    if (boot_sector.bytes_per_sector != 512) return;

    fat_start_sector = boot_sector.reserved_sectors;
    root_start_sector = fat_start_sector + (boot_sector.num_fats * boot_sector.fat_size_16);
    root_sectors = ((boot_sector.root_entries * 32) + (boot_sector.bytes_per_sector - 1)) / boot_sector.bytes_per_sector;
    data_start_sector = root_start_sector + root_sectors;
    
    uint32_t total_sectors = (boot_sector.total_sectors_16 == 0) ? boot_sector.total_sectors_32 : boot_sector.total_sectors_16;
    uint32_t data_sectors = total_sectors - data_start_sector;
    uint32_t total_clusters = data_sectors / boot_sector.sectors_per_cluster;
    
    if (total_clusters < 4085) {
        fat_type = 12;
    } else {
        fat_type = 16;
    }
}

static uint32_t get_fat_entry(uint32_t cluster) {
    uint32_t fat_offset;
    if (fat_type == 12) {
        fat_offset = cluster + (cluster / 2);
    } else {
        fat_offset = cluster * 2;
    }

    uint32_t sector = fat_start_sector + (fat_offset / 512);
    uint32_t offset = fat_offset % 512;

    uint8_t buf[512];
    if (ata_read_sector(sector, buf) != 0) return 0x0FFFFFF7;

    uint32_t next_cluster;
    if (fat_type == 12) {
        next_cluster = (uint32_t)(*(uint16_t*)&buf[offset]);
        if (cluster & 0x0001) {
            next_cluster >>= 4;
        } else {
            next_cluster &= 0x0FFF;
        }
    } else {
        next_cluster = (uint32_t)(*(uint16_t*)&buf[offset]);
    }

    return next_cluster;
}

static int filename_to_fat(const char *name, char *fat_name) {
    memset(fat_name, ' ', 11);
    int i = 0;
    int j = 0;
    
    while (name[i] && name[i] != '.' && j < 8) {
        fat_name[j++] = (name[i] >= 'a' && name[i] <= 'z') ? name[i] - 'a' + 'A' : name[i];
        i++;
    }

    if (name[i] == '.') {
        i++;
        j = 8;
        while (name[i] && j < 11) {
            fat_name[j++] = (name[i] >= 'a' && name[i] <= 'z') ? name[i] - 'a' + 'A' : name[i];
            i++;
        }
    }
    return 0;
}

static fat_dirent_t* find_dirent(const char *name) {
    char fat_name[11];
    filename_to_fat(name, fat_name);
    
    uint8_t buf[512];
    for (uint32_t s = 0; s < root_sectors; s++) {
        if (ata_read_sector(root_start_sector + s, buf) != 0) break;

        fat_dirent_t *entries = (fat_dirent_t*)buf;
        for (int i = 0; i < 16; i++) {
            if ((uint8_t)entries[i].name[0] == 0x00) return NULL;
            if ((uint8_t)entries[i].name[0] == 0xE5) continue;
            if (entries[i].attr & FAT_ATTR_VOLUME_ID) continue;

            if (memcmp(entries[i].name, fat_name, 11) == 0) {
                fat_dirent_t *res = kmalloc(sizeof(fat_dirent_t));
                memcpy(res, &entries[i], sizeof(fat_dirent_t));
                return res;
            }
        }
    }
    return NULL;
}

int fat_file_exists(const char *name) {
    fat_dirent_t *ent = find_dirent(name);
    if (ent) {
        kfree(ent);
        return 1;
    }
    return 0;
}

void fat_load_file(const char *name, void *buf) {
    fat_dirent_t *ent = find_dirent(name);
    if (!ent) {
        memset(buf, 0, 512);
        return;
    }

    uint32_t cluster = ent->cluster_low | (ent->cluster_high << 16);
    uint8_t *ptr = (uint8_t*)buf;
    uint32_t bytes_left = ent->size;

    while (cluster < (fat_type == 12 ? 0x0FF8 : 0xFFF8) && bytes_left > 0) {
        uint32_t sector = data_start_sector + (cluster - 2) * boot_sector.sectors_per_cluster;

        for (int s = 0; s < boot_sector.sectors_per_cluster && bytes_left > 0; s++) {
            uint8_t sector_buf[512];
            ata_read_sector(sector + s, sector_buf);

            uint32_t to_copy = (bytes_left > 512) ? 512 : bytes_left;
            memcpy(ptr, sector_buf, to_copy);

            ptr += to_copy;
            bytes_left -= to_copy;
        }

        cluster = get_fat_entry(cluster);
    }

    kfree(ent);
}

void fat_save_file(const char *name, void *buf) {
    /* TODO: Real write support */
    UNUSED(name);
    UNUSED(buf);
}

void fat_list_files(void) {
    uint8_t buf[512];
    shell_print("\nName         Size       Attr\n");
    shell_print("-----------  ---------  ----\n");

    for (uint32_t s = 0; s < root_sectors; s++) {
        if (ata_read_sector(root_start_sector + s, buf) != 0) break;

        fat_dirent_t *entries = (fat_dirent_t*)buf;
        for (int i = 0; i < 16; i++) {
            if ((uint8_t)entries[i].name[0] == 0x00) return;
            if ((uint8_t)entries[i].name[0] == 0xE5) continue;
            if (entries[i].attr & FAT_ATTR_VOLUME_ID) continue;

            /* Print name.ext */
            char name[13];
            int p = 0;
            for (int k = 0; k < 8; k++) if (entries[i].name[k] != ' ') name[p++] = entries[i].name[k];
            if (entries[i].ext[0] != ' ') {
                name[p++] = '.';
                for (int k = 0; k < 3; k++) if (entries[i].ext[k] != ' ') name[p++] = entries[i].ext[k];
            }
            name[p] = 0;

            shell_print(name);
            for (int k = strlen(name); k < 13; k++) shell_print(" ");

            shell_print_int(entries[i].size);
            shell_print(" bytes    ");

            if (entries[i].attr & FAT_ATTR_DIRECTORY) shell_print("DIR");
            else shell_print("FILE");
            shell_print("\n");
        }
    }
}
