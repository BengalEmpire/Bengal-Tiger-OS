#include "paging.h"
#include "common.h"

#define BITMAP_SIZE 32768
static uint32_t bitmap[BITMAP_SIZE];
static uint32_t total_frames;

void pmm_init(uint32_t mem_size, uint32_t kernel_end) {
    total_frames = mem_size / PAGE_SIZE;
    memset(bitmap, 0xFF, sizeof(bitmap));
    
    // Mark kernel memory as used
    for (uint32_t addr = kernel_end; addr < mem_size; addr += PAGE_SIZE) {
        uint32_t frame = addr / PAGE_SIZE;
        // Simple bitmap logic
        if (frame / 32 < BITMAP_SIZE) {
            bitmap[frame / 32] &= ~(1 << (frame % 32));
        }
    }
}

uint32_t pmm_alloc_frame() {
    for (uint32_t i = 0; i < BITMAP_SIZE; i++) {
        if (bitmap[i] != 0xFFFFFFFF) {
            for (uint32_t j = 0; j < 32; j++) {
                // Find first free bit (0)
                if (!(bitmap[i] & (1 << j))) {
                    uint32_t frame = i * 32 + j;
                    bitmap[i] |= (1 << j);
                    return frame * PAGE_SIZE;
                }
            }
        }
    }
    return 0; // Out of memory
}

void pmm_free_frame(uint32_t addr) {
    uint32_t frame = addr / PAGE_SIZE;
    if (frame / 32 < BITMAP_SIZE) {
        bitmap[frame / 32] &= ~(1 << (frame % 32));
    }
}

// 4MB Page Directory (1024 * 4KB = 4MB)
uint32_t *page_dir = (uint32_t*)0x9C000;  
uint32_t *page_table = (uint32_t*)0x9D000;

void paging_install(uint32_t mem_size) {
    (void)mem_size; // Silence unused parameter warning

    // 1. Identity map the first 4MB (so the kernel code matches physical RAM)
    for (uint32_t i = 0; i < 1024; i++) {
        // | 3 means: Present (1) | Read/Write (2) = 3
        page_table[i] = (i * PAGE_SIZE) | 3;
    }

    // 2. Put the page table into the page directory (Entry 0)
    page_dir[0] = ((uint32_t)page_table) | 3;

    // 3. Clear the rest of the directory
    for (uint32_t i = 1; i < 1024; i++) {
        page_dir[i] = 0 | 2; // Attribute: Read/Write, but not present
    }

    // 4. Load CR3 with the address of the page directory
    __asm__ volatile("mov %0, %%cr3" : : "r"(page_dir));

    // 5. Enable Paging in CR0
    uint32_t cr0;
    __asm__ volatile("mov %%cr0, %0" : "=r"(cr0));
    cr0 |= 0x80000000; // Set PG bit (31)
    __asm__ volatile("mov %0, %%cr0" : : "r"(cr0));
}