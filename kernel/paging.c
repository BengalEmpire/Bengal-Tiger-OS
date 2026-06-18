/**
 * Bengal Tiger OS - Memory Management Implementation
 * 
 * Physical Memory Manager using bitmap allocation.
 * Paging with identity mapping for first 4MB.
 * 
 * @file paging.c
 * @author Bengal Tiger OS (BengalEmpire)
 * @version 0.3.0
 */

#include "paging.h"
#include "common.h"

/* ============================================ */
/* Physical Memory Manager                      */
/* ============================================ */

/* Bitmap: each bit represents one 4KB page frame 
 * 32768 * 32 bits = 1,048,576 frames = 4GB max addressable */
#define BITMAP_SIZE 32768
static uint32_t bitmap[BITMAP_SIZE];

/* Memory statistics */
static uint32_t total_frames = 0;
static uint32_t used_frames = 0;

void pmm_init(uint32_t mem_size, uint32_t kernel_end) {
    total_frames = mem_size / PAGE_SIZE;
    
    /* Initially mark all frames as used (1 = used) */
    memset(bitmap, 0xFF, sizeof(bitmap));
    
    /* Calculate first usable frame (after kernel) */
    uint32_t first_free = ALIGN_UP(kernel_end, PAGE_SIZE);
    
    /* Mark frames from first_free to mem_size as free */
    for (uint32_t addr = first_free; addr < mem_size; addr += PAGE_SIZE) {
        uint32_t frame = addr / PAGE_SIZE;
        if (frame / 32 < BITMAP_SIZE) {
            bitmap[frame / 32] &= ~(1 << (frame % 32));  /* 0 = free */
        }
    }
    
    /* Calculate used frames (kernel + reserved) */
    used_frames = first_free / PAGE_SIZE;
}

uint32_t pmm_alloc_frame(void) {
    /* Search for first free bit (0) in bitmap */
    for (uint32_t i = 0; i < BITMAP_SIZE; i++) {
        if (bitmap[i] != 0xFFFFFFFF) {
            /* There's at least one free bit in this word */
            for (uint32_t j = 0; j < 32; j++) {
                if (!(bitmap[i] & (1 << j))) {
                    /* Found free frame */
                    uint32_t frame = i * 32 + j;
                    
                    /* Bounds check */
                    if (frame >= total_frames) {
                        return 0;  /* Out of memory */
                    }
                    
                    /* Mark as used */
                    bitmap[i] |= (1 << j);
                    used_frames++;
                    
                    return frame * PAGE_SIZE;
                }
            }
        }
    }
    
    return 0;  /* Out of memory */
}

void pmm_free_frame(uint32_t addr) {
    uint32_t frame = addr / PAGE_SIZE;
    
    if (frame / 32 < BITMAP_SIZE && frame < total_frames) {
        /* Only free if currently allocated */
        if (bitmap[frame / 32] & (1 << (frame % 32))) {
            bitmap[frame / 32] &= ~(1 << (frame % 32));
            used_frames--;
        }
    }
}

uint32_t pmm_get_total_frames(void) {
    return total_frames;
}

uint32_t pmm_get_used_frames(void) {
    return used_frames;
}

/* ============================================ */
/* Paging Setup                                 */
/* ============================================ */

/* Page Directory and first Page Table pointers */
static uint32_t *page_dir = NULL;
static uint32_t *page_table0 = NULL;

void paging_install(uint32_t mem_size) {
    /* 
     * Dynamically allocate page directory and first page table
     * right after the kernel’s BSS section.
     * This is MUCH safer than hardcoding 0x9C000 which could
     * overlap with GRUB modules, ACPI tables, or BIOS data.
     */
    extern uint32_t bss_end;
    uint32_t kernel_end = (uint32_t)&bss_end;
    kernel_end = ALIGN_UP(kernel_end, PAGE_SIZE);
    
    page_dir    = (uint32_t*)kernel_end;
    page_table0 = (uint32_t*)(kernel_end + PAGE_SIZE);
    
    /* 1. Clear page directory */
    memset(page_dir, 0, PAGE_SIZE);
    
    /* 2. Identity map first 4MB (1024 pages x 4KB = 4MB) */
    for (uint32_t i = 0; i < 1024; i++) {
        /* Each entry: Physical Address | Present | Writable */
        page_table0[i] = (i * PAGE_SIZE) | PAGE_PRESENT | PAGE_WRITABLE;
    }
    
    /* 3. Set page directory entry 0 to point to page_table0 */
    page_dir[0] = (uint32_t)page_table0 | PAGE_PRESENT | PAGE_WRITABLE;
    
    /* 4. Clear remaining page directory entries (not present) */
    for (uint32_t i = 1; i < 1024; i++) {
        page_dir[i] = PAGE_WRITABLE;  /* Not present, but writable if mapped later */
    }
    
    /* 5. Load CR3 with page directory address */
    __asm__ volatile("mov %0, %%cr3" : : "r"(page_dir) : "memory");
    
    /* 6. Enable paging bit in CR0 */
    uint32_t cr0;
    __asm__ volatile("mov %%cr0, %0" : "=r"(cr0));
    cr0 |= 0x80000000;  /* Set PG bit (bit 31) */
    __asm__ volatile("mov %0, %%cr0" : : "r"(cr0) : "memory");
}

void paging_map(uint32_t virt, uint32_t phys, uint32_t flags) {
    uint32_t pd_index = virt >> 22;
    uint32_t pt_index = (virt >> 12) & 0x3FF;
    
    /* Get or create page table for this region */
    uint32_t *pt;
    
    if (page_dir[pd_index] & PAGE_PRESENT) {
        pt = (uint32_t*)(page_dir[pd_index] & ~0xFFF);
    } else {
        /* Allocate new page table */
        pt = (uint32_t*)pmm_alloc_frame();
        if (pt == NULL) return;
        
        memset(pt, 0, PAGE_SIZE);
        page_dir[pd_index] = (uint32_t)pt | PAGE_PRESENT | PAGE_WRITABLE | (flags & PAGE_USER);
    }
    
    /* Set page table entry */
    pt[pt_index] = (phys & ~0xFFF) | flags;
    
    /* Invalidate TLB for this address */
    paging_invalidate(virt);
}

void paging_unmap(uint32_t virt) {
    uint32_t pd_index = virt >> 22;
    uint32_t pt_index = (virt >> 12) & 0x3FF;
    
    if (!(page_dir[pd_index] & PAGE_PRESENT)) {
        return;  /* Page table doesn't exist */
    }
    
    uint32_t *pt = (uint32_t*)(page_dir[pd_index] & ~0xFFF);
    pt[pt_index] = 0;  /* Clear entry */
    
    paging_invalidate(virt);
}

uint32_t paging_get_physical(uint32_t virt) {
    uint32_t pd_index = virt >> 22;
    uint32_t pt_index = (virt >> 12) & 0x3FF;
    
    if (!(page_dir[pd_index] & PAGE_PRESENT)) {
        return 0;
    }
    
    uint32_t *pt = (uint32_t*)(page_dir[pd_index] & ~0xFFF);
    
    if (!(pt[pt_index] & PAGE_PRESENT)) {
        return 0;
    }
    
    return (pt[pt_index] & ~0xFFF) | (virt & 0xFFF);
}

void paging_flush_tlb(void) {
    /* Reload CR3 to flush entire TLB */
    uint32_t cr3;
    __asm__ volatile("mov %%cr3, %0" : "=r"(cr3));
    __asm__ volatile("mov %0, %%cr3" : : "r"(cr3) : "memory");
}