/**
 * Bengal Tiger OS - FAT Filesystem Driver (Stub)
 * 
 * @file fat.c
 * @author Bengal Tiger OS (BengalEmpire)
 * @version 0.3.0
 */

#include "fat.h"
#include "disk.h"
#include "common.h"

/* FAT Boot Sector Structure (partial) */
typedef struct {
    uint8_t  jump[3];           /* Jump instruction */
    char     oem[8];            /* OEM name */
    uint16_t bytes_per_sector;  /* Bytes per sector (512) */
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
} __attribute__((packed)) fat_boot_t;

/* Simple file table (hardcoded for demo) */
static fat_file_t files[] = {
    {"hello.txt",   0, 28, 1},    /* Sector 1 */
    {"config.cfg",  0, 512, 2},   /* Sector 2 */
    {"", 0, 0, 0}                 /* End marker */
};

static int file_count = 2;

void fat_init(void) {
    /* Read boot sector for reference (not fully parsed) */
    uint8_t boot_sector[512];
    ata_read_sector(0, boot_sector);
    
    /* 
     * TODO: Parse boot sector and locate:
     * - FAT tables
     * - Root directory
     * - Data area
     */
    
    /* For now, using hardcoded sector mapping */
}

static fat_file_t* fat_find_file(const char *name) {
    for (int i = 0; i < file_count; i++) {
        if (strcmp(files[i].name, name) == 0) {
            return &files[i];
        }
    }
    return NULL;
}

void fat_load_file(const char *name, void *buf) {
    fat_file_t *file = fat_find_file(name);
    
    if (file != NULL && file->sector > 0) {
        ata_read_sector(file->sector, (uint8_t*)buf);
    } else {
        /* File not found - zero the buffer */
        memset(buf, 0, 512);
    }
}

void fat_save_file(const char *name, void *buf) {
    fat_file_t *file = fat_find_file(name);
    
    if (file != NULL && file->sector > 0) {
        ata_write_sector(file->sector, (uint8_t*)buf);
    }
    /* 
     * TODO: For new files, need to:
     * - Find free directory entry
     * - Find free cluster
     * - Update FAT table
     * - Write data
     */
}

int fat_file_exists(const char *name) {
    return fat_find_file(name) != NULL;
}

void fat_list_files(void) {
    /* Would iterate through root directory */
    /* For now, just uses hardcoded list */
}