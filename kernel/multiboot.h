/**
 * Bengal Tiger OS - Multiboot Information Structures
 *
 * Full definitions for parsing the multiboot information structure
 * passed by GRUB in EBX. Enables proper memory map parsing,
 * module loading, and ELF section detection.
 *
 * @file multiboot.h
 * @author Bengal Tiger OS (BengalEmpire)
 * @version 0.4.0
 */

#ifndef MULTIBOOT_H
#define MULTIBOOT_H

#include "common.h"

/* ============================================ */
/* Multiboot Header Magic & Flags               */
/* ============================================ */

#define MULTIBOOT_MAGIC         0x1BADB002
#define MULTIBOOT_BOOTLOADER_MAGIC 0x2BADB002

/* ============================================ */
/* Multiboot Memory Map Entry                   */
/* ============================================ */

/** Memory region types */
#define MULTIBOOT_MMAP_AVAILABLE        1
#define MULTIBOOT_MMAP_RESERVED         2
#define MULTIBOOT_MMAP_ACPI_RECLAIMABLE 3
#define MULTIBOOT_MMAP_ACPI_NVS         4
#define MULTIBOOT_MMAP_BADRAM           5

/** Memory map entry from GRUB */
typedef struct {
    uint32_t size;          /* Size of this entry (minus size field) */
    uint64_t addr;          /* Base address */
    uint64_t len;           /* Length of region */
    uint32_t type;          /* Type (1 = available, 2 = reserved, etc.) */
} __attribute__((packed)) multiboot_mmap_entry_t;

/* ============================================ */
/* Multiboot Module Info                        */
/* ============================================ */

typedef struct {
    uint32_t mod_start;
    uint32_t mod_end;
    uint32_t cmdline;
    uint32_t reserved;
} __attribute__((packed)) multiboot_module_t;

/* ============================================ */
/* Multiboot ELF Section Info                   */
/* ============================================ */

typedef struct {
    uint32_t num;
    uint32_t size;
    uint32_t addr;
    uint32_t shndx;
} __attribute__((packed)) multiboot_elf_section_header_table_t;

/* ============================================ */
/* Multiboot Full Information Structure         */
/* ============================================ */

/**
 * Full multiboot information structure.
 * Only fields with the corresponding flag bit set are valid.
 */
typedef struct {
    /* Flags (which fields are valid) */
    uint32_t flags;

    /* Available memory from BIOS */
    uint32_t mem_lower;         /* [0] Lower memory in KB */
    uint32_t mem_upper;         /* [0] Upper memory in KB */

    /* Boot device info */
    uint32_t boot_device;       /* [1] Boot device */

    /* Command line */
    uint32_t cmdline;           /* [2] Kernel command line string */

    /* Boot modules */
    uint32_t mods_count;        /* [3] Number of boot modules */
    uint32_t mods_addr;         /* [3] Address of module array */

    /* ELF / a.out symbol table */
    union {
        multiboot_elf_section_header_table_t elf_sec;
        uint32_t syms[4];
    } syms;                     /* [4-5] Symbol table info */

    /* Memory map */
    uint32_t mmap_length;       /* [6] Length of memory map */
    uint32_t mmap_addr;         /* [6] Address of memory map */

    /* Drives info */
    uint32_t drives_length;     /* [7] Length of drives info */
    uint32_t drives_addr;       /* [7] Address of drives info */

    /* ROM configuration table */
    uint32_t config_table;      /* [8] ROM configuration table */

    /* Boot loader name */
    uint32_t boot_loader_name;  /* [9] Boot loader name string */

    /* APM table */
    uint32_t apm_table;         /* [10] APM table */

    /* VBE info */
    uint32_t vbe_control_info;  /* [11] VBE controller info */
    uint32_t vbe_mode_info;     /* [11] VBE mode info */
    uint16_t vbe_mode;          /* [11] VBE current mode */
    uint16_t vbe_interface_seg; /* [11] VBE interface segment */
    uint16_t vbe_interface_off; /* [11] VBE interface offset */
    uint16_t vbe_interface_len; /* [11] VBE interface length */

    /* Framebuffer info (VBE 3.0+) */
    uint64_t framebuffer_addr;  /* [12] Framebuffer address */
    uint32_t framebuffer_pitch; /* [12] Bytes per scanline */
    uint32_t framebuffer_width; /* [12] Width in pixels */
    uint32_t framebuffer_height;/* [12] Height in pixels */
    uint8_t  framebuffer_bpp;   /* [12] Bits per pixel */
    uint8_t  framebuffer_type;  /* [12] Framebuffer type */
    uint8_t  framebuffer_red_field_position;   /* [12] */
    uint8_t  framebuffer_red_mask_size;        /* [12] */
    uint8_t  framebuffer_green_field_position; /* [12] */
    uint8_t  framebuffer_green_mask_size;      /* [12] */
    uint8_t  framebuffer_blue_field_position;  /* [12] */
    uint8_t  framebuffer_blue_mask_size;       /* [12] */
    uint8_t  framebuffer_reserved_field_position; /* [12] */
    uint8_t  framebuffer_reserved_mask_size;   /* [12] */
} __attribute__((packed)) multiboot_info_t;

/* ============================================ */
/* Helper Functions (inline)                    */
/* ============================================ */

/**
 * Get total physical memory from multiboot info.
 * Returns in bytes.
 */
static inline uint32_t multiboot_total_memory(multiboot_info_t *mbi) {
    if (mbi->flags & (1 << 0)) {
        return (mbi->mem_upper * 1024) + (mbi->mem_lower * 1024);
    }
    return 16 * 1024 * 1024;  /* Assume 16MB if not provided */
}

/**
 * Iterate through memory map entries.
 * Returns the first entry, or NULL if none.
 */
static inline multiboot_mmap_entry_t* multiboot_mmap_first(multiboot_info_t *mbi) {
    if (!(mbi->flags & (1 << 6)) || mbi->mmap_length == 0) {
        return NULL;
    }
    return (multiboot_mmap_entry_t*)mbi->mmap_addr;
}

/**
 * Get the next memory map entry.
 */
static inline multiboot_mmap_entry_t* multiboot_mmap_next(multiboot_mmap_entry_t *entry) {
    return (multiboot_mmap_entry_t*)((uint32_t)entry + entry->size + sizeof(entry->size));
}

/**
 * Check if there are more entries.
 */
static inline int multiboot_mmap_has_more(multiboot_info_t *mbi, multiboot_mmap_entry_t *entry) {
    return (uint32_t)entry < (mbi->mmap_addr + mbi->mmap_length);
}

#endif /* MULTIBOOT_H */
