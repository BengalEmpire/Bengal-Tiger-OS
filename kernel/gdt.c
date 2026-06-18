/**
 * Bengal Tiger OS - GDT Implementation
 *
 * Sets up a flat 32-bit memory model with kernel code/data segments.
 * Implements all GDT entry management and loading with segment register
 * reload via inline assembly (far jump to reload CS).
 *
 * @file gdt.c
 * @author Bengal Tiger OS (BengalEmpire)
 * @version 0.4.0
 */

#include "gdt.h"
#include "common.h"

/* GDT entries: null, kernel code, kernel data */
#define GDT_ENTRIES 3

/* Static GDT table */
static gdt_entry_t gdt_entries[GDT_ENTRIES];
static gdt_ptr_t   gdt_ptr;

/**
 * Set a GDT entry with the given parameters.
 *
 * @param num      Entry index
 * @param base     Segment base address
 * @param limit    Segment limit (max 0xFFFFF with 4K granularity = 4GB)
 * @param access   Access control byte
 * @param gran     Granularity and flags byte
 */
static void gdt_set_entry(int num, uint32_t base, uint32_t limit,
                          uint8_t access, uint8_t gran) {
    gdt_entries[num].base_low     = base & 0xFFFF;
    gdt_entries[num].base_mid     = (base >> 16) & 0xFF;
    gdt_entries[num].base_high    = (base >> 24) & 0xFF;

    gdt_entries[num].limit_low    = limit & 0xFFFF;
    gdt_entries[num].granularity  = (limit >> 16) & 0x0F;
    gdt_entries[num].granularity |= gran & 0xF0;

    gdt_entries[num].access       = access;
}

/**
 * Load the GDT and reload segment registers.
 *
 * Uses inline assembly to:
 *   1. Load the GDTR with lgdt
 *   2. Reload data segments (DS, ES, FS, GS, SS)
 *   3. Far jump to reload the code segment (CS)
 *
 * This is critical — after loading a new GDT, ALL segment registers
 * must be reloaded with the new selectors.
 */
static void gdt_load(void) {
    __asm__ volatile(
        /* Load GDTR */
        "lgdt %0\n"
        /* Reload data segments */
        "mov %1, %%ax\n"
        "mov %%ax, %%ds\n"
        "mov %%ax, %%es\n"
        "mov %%ax, %%fs\n"
        "mov %%ax, %%gs\n"
        "mov %%ax, %%ss\n"
        /* Far jump to reload CS (code segment) */
        "ljmp %2, $1f\n"
        "1:\n"
        :
        : "m"(gdt_ptr),
          "i"(GDT_KERNEL_DATA),
          "i"(GDT_KERNEL_CODE)
        : "eax", "memory"
    );
}

/**
 * Initialize and load the Global Descriptor Table.
 *
 * Creates three entries:
 *   0: Null descriptor (required by CPU for security)
 *   1: Kernel Code segment — Ring 0, 32-bit, base 0, limit 4GB
 *   2: Kernel Data segment — Ring 0, 32-bit, base 0, limit 4GB
 *
 * After setup, loads the GDT and reloads all segment registers.
 * This function should be called very early in kmain(), before
 * any interrupt or paging initialization.
 */
void gdt_init(void) {
    /* Clear GDT entries */
    memset(gdt_entries, 0, sizeof(gdt_entries));

    /* Set up the pointer */
    gdt_ptr.limit = sizeof(gdt_entry_t) * GDT_ENTRIES - 1;
    gdt_ptr.base  = (uint32_t)&gdt_entries;

    /* Null descriptor (index 0) — already zeroed */

    /* Kernel Code segment (index 1)
     *   Base: 0, Limit: 4GB, 32-bit, Ring 0, Executable, Readable */
    gdt_set_entry(1, 0, 0xFFFFF,
                  GDT_KERNEL_CODE_ACCESS,
                  GDT_KERNEL_CODE_FLAGS);

    /* Kernel Data segment (index 2)
     *   Base: 0, Limit: 4GB, 32-bit, Ring 0, Data, Writable */
    gdt_set_entry(2, 0, 0xFFFFF,
                  GDT_KERNEL_DATA_ACCESS,
                  GDT_KERNEL_DATA_FLAGS);

    /* Load the GDT and reload segments */
    gdt_load();
}
