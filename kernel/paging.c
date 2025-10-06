// kernel/paging.c

#include "paging.h"
#include "common.h"

#define BITMAP_SIZE 32768
static uint32_t bitmap[BITMAP_SIZE];
static uint32_t total_frames;

void pmm_init(uint32_t mem_size, uint32_t kernel_end) {
    total_frames = mem_size / PAGE_SIZE;
    memset(bitmap, 0xFF, sizeof(bitmap));
    for (uint32_t addr = kernel_end; addr < mem_size; addr += PAGE_SIZE) {
        uint32_t frame = addr / PAGE_SIZE;
        if (frame / 32 < BITMAP_SIZE) {
            bitmap[frame / 32] &= ~(1 << (frame % 32));
        }
    }
}

uint32_t pmm_alloc_frame() {
    for (uint32_t i = 0; i < BITMAP_SIZE; i++) {
        if (bitmap[i] != 0xFFFFFFFF) {
            for (uint32_t j = 0; j < 32; j++) {
                if (!(bitmap[i] & (1 << j))) {
                    uint32_t frame = i * 32 + j;
                    bitmap[i] |= (1 << j);
                    return frame * PAGE_SIZE;
                }
            }
        }
    }
    return 0;
}

void pmm_free_frame(uint32_t addr) {
    uint32_t frame = addr / PAGE_SIZE;
    if (frame / 32 < BITMAP_SIZE) {
        bitmap[frame / 32] &= ~(1 << (frame % 32));
    }
}

uint32_t *page_dir = (uint32_t*)0x1000;
uint32_t *page_table = (uint32_t*)0x2000;

void paging_install(uint32_t mem_size) {
    for (uint32_t i = 0; i < 1024; i++) {
        page_table[i] = (i * PAGE_SIZE) | 3;
    }
    page_dir[0] = ((uint32_t)page_table) | 3;
    for (uint32_t i = 1; i < 1024; i++) page_dir[i] = 0;

    __asm__ volatile("mov %0, %%cr3" : : "r"(page_dir));
    uint32_t cr0;
    __asm__ volatile("mov %%cr0, %0" : "=r"(cr0));
    cr0 |= 0x80000000;
    __asm__ volatile("mov %0, %%cr0" : : "r"(cr0));
}