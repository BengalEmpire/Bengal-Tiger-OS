/**
 * Bengal Tiger OS - Full FAT12/FAT16 Filesystem Driver
 *
 * Supports Superfloppy & MBR-partitioned FAT12/FAT16 volumes, automatic
 * Live RAMDisk fallback on ISO/USB boot, `mkfs` volume formatting,
 * multi-cluster reading/writing/appending, file deletion, copy, rename,
 * filesystem capacity reporting (`df`), and filename tab-completion.
 *
 * @file fat.h
 * @author Bengal Tiger OS (BengalEmpire)
 * @version 1.0.0
 */

#ifndef FAT_H
#define FAT_H

#include "common.h"

/* FAT Directory Attributes */
#define FAT_ATTR_READ_ONLY  0x01
#define FAT_ATTR_HIDDEN     0x02
#define FAT_ATTR_SYSTEM     0x04
#define FAT_ATTR_VOLUME_ID  0x08
#define FAT_ATTR_DIRECTORY  0x10
#define FAT_ATTR_ARCHIVE    0x20
#define FAT_ATTR_LFN        0x0F

#define FAT_MAX_FILE_SIZE   (256 * 1024) /* 256 KB max single-buffer file op */

typedef struct {
    char     name[13];      /* Null-terminated 8.3 filename */
    uint8_t  attr;
    uint32_t size;
    uint32_t first_cluster;
} fat_file_t;

typedef struct {
    int      mounted;
    int      fat_type;          /* 12 or 16 */
    int      is_ramdisk;        /* 1 = Live RAMDisk, 0 = Hardware ATA */
    uint32_t volume_start_lba;
    uint32_t bytes_per_cluster;
    uint32_t total_clusters;
    uint32_t used_clusters;
    uint32_t free_clusters;
    uint32_t total_bytes;
    uint32_t used_bytes;
    uint32_t free_bytes;
    uint32_t file_count;
    char     label[12];
} fat_fs_info_t;

void fat_init(void);
int  fat_is_mounted(void);
int  fat_format(const char *label, int use_hardware_ata);

int  fat_read_file(const char *name, void *buf, uint32_t max_len, uint32_t *out_size);
int  fat_write_file(const char *name, const void *buf, uint32_t len);
int  fat_append_file(const char *name, const void *buf, uint32_t len);
int  fat_delete_file(const char *name);
int  fat_copy_file(const char *src, const char *dst);
int  fat_rename_file(const char *old_name, const char *new_name);
int  fat_stat_file(const char *name, fat_file_t *out_info);
void fat_get_fs_info(fat_fs_info_t *out_info);
int  fat_enumerate_files(char names[][16], int max_files);

/* Classic compatibility helpers */
void fat_load_file(const char *name, void *buf);
void fat_save_file(const char *name, const void *buf);
int  fat_file_exists(const char *name);
void fat_list_files(void);

#endif /* FAT_H */
