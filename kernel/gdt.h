/**
 * Bengal Tiger OS - Global Descriptor Table
 *
 * Provides a flat 32-bit memory model with:
 *   - Null descriptor (required)
 *   - Kernel Code: Ring 0, 32-bit, base 0, limit 4GB
 *   - Kernel Data: Ring 0, 32-bit, base 0, limit 4GB
 *
 * This replaces the temporary GDT set up by GRUB, ensuring
 * we have full control over segment descriptors.
 *
 * @file gdt.h
 * @author Bengal Tiger OS (BengalEmpire)
 * @version 0.4.0
 */

#ifndef GDT_H
#define GDT_H

#include "common.h"

/* GDT Segment Selectors */
#define GDT_NULL_SEG     0x00  /* Null descriptor (required) */
#define GDT_KERNEL_CODE  0x08  /* Kernel code segment (Ring 0) */
#define GDT_KERNEL_DATA  0x10  /* Kernel data segment (Ring 0) */

/* Access byte masks */
#define GDT_ACCESS_PRESENT       0x80  /* Segment present */
#define GDT_ACCESS_RING0         0x00  /* Privilege level 0 */
#define GDT_ACCESS_RING3         0x60  /* Privilege level 3 */
#define GDT_ACCESS_CODE          0x18  /* Code segment (vs data) */
#define GDT_ACCESS_DATA          0x10  /* Data segment (vs code) */
#define GDT_ACCESS_READ_WRITE    0x02  /* Readable (code) / Writable (data) */
#define GDT_ACCESS_EXECUTABLE    0x08  /* Executable (code segments) */
#define GDT_ACCESS_DIRECTION     0x04  /* Direction (data) / Conforming (code) */

/* Granularity byte masks */
#define GDT_FLAG_GRANULARITY_4K  0x80  /* 4KB page granularity */
#define GDT_FLAG_GRANULARITY_1B  0x00  /* 1 byte granularity */
#define GDT_FLAG_32BIT           0x40  /* 32-bit protected mode */
#define GDT_FLAG_16BIT           0x00  /* 16-bit protected mode */

/* Common descriptor flags */
#define GDT_KERNEL_CODE_FLAGS    (GDT_FLAG_GRANULARITY_4K | GDT_FLAG_32BIT)
#define GDT_KERNEL_DATA_FLAGS    (GDT_FLAG_GRANULARITY_4K | GDT_FLAG_32BIT)

/* Common descriptor access */
#define GDT_KERNEL_CODE_ACCESS   (GDT_ACCESS_PRESENT | GDT_ACCESS_RING0 | \
                                  GDT_ACCESS_CODE | GDT_ACCESS_READ_WRITE | \
                                  GDT_ACCESS_EXECUTABLE)
#define GDT_KERNEL_DATA_ACCESS   (GDT_ACCESS_PRESENT | GDT_ACCESS_RING0 | \
                                  GDT_ACCESS_DATA | GDT_ACCESS_READ_WRITE)

/**
 * GDT entry structure (8 bytes).
 * Each entry describes one segment.
 */
typedef struct {
    uint16_t limit_low;     /* Lower 16 bits of segment limit */
    uint16_t base_low;      /* Lower 16 bits of base address */
    uint8_t  base_mid;      /* Next 8 bits of base address */
    uint8_t  access;        /* Access flags (present, ring, type) */
    uint8_t  granularity;   /* Granularity flags + upper 4 bits of limit */
    uint8_t  base_high;     /* Upper 8 bits of base address */
} __attribute__((packed)) gdt_entry_t;

/**
 * GDT pointer structure (loaded into GDTR).
 */
typedef struct {
    uint16_t limit;         /* Size of GDT in bytes - 1 */
    uint32_t base;          /* Linear address of GDT */
} __attribute__((packed)) gdt_ptr_t;

/**
 * TSS entry structure (in GDT, for future Ring 3 support).
 */
typedef struct {
    uint16_t limit_low;
    uint16_t base_low;
    uint8_t  base_mid;
    uint8_t  access;
    uint8_t  granularity;
    uint8_t  base_high;
    uint32_t base_upper;
    uint32_t reserved;
} __attribute__((packed)) tss_entry_t;

/**
 * Initialize and load the Global Descriptor Table.
 * Creates a flat 32-bit memory model with proper segment descriptors.
 * Reloads all segment registers and performs a far jump to reload CS.
 */
void gdt_init(void);

#endif /* GDT_H */
