/**
 * Bengal Tiger OS - FAT Filesystem Driver (Stub)
 * 
 * Simplified filesystem interface.
 * Currently uses fixed sector mapping.
 * 
 * @file fat.h
 * @author Bengal Tiger OS (BengalEmpire)
 * @version 0.3.0
 */

#ifndef FAT_H
#define FAT_H

#include "common.h"

/* File attributes */
#define FAT_ATTR_READ_ONLY  0x01
#define FAT_ATTR_HIDDEN     0x02
#define FAT_ATTR_SYSTEM     0x04
#define FAT_ATTR_VOLUME_ID  0x08
#define FAT_ATTR_DIRECTORY  0x10
#define FAT_ATTR_ARCHIVE    0x20

/* File entry (simplified) */
typedef struct {
    char name[12];          /* 8.3 filename */
    uint8_t attr;           /* File attributes */
    uint32_t size;          /* File size in bytes */
    uint32_t sector;        /* Starting sector */
} fat_file_t;

/**
 * Initialize FAT filesystem.
 * Reads boot sector and sets up structures.
 */
void fat_init(void);

/**
 * Load file contents into buffer.
 * 
 * @param name Filename to load
 * @param buf Buffer to load into (at least 512 bytes)
 */
void fat_load_file(const char *name, void *buf);

/**
 * Save data to a file.
 * 
 * @param name Filename to save to
 * @param buf Data to save (512 bytes)
 */
void fat_save_file(const char *name, void *buf);

/**
 * Check if a file exists.
 * 
 * @param name Filename to check
 * @return 1 if exists, 0 if not
 */
int fat_file_exists(const char *name);

/**
 * List files in root directory (stub).
 */
void fat_list_files(void);

#endif /* FAT_H */