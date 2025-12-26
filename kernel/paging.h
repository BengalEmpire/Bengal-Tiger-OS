/**
 * Bengal Tiger OS - Memory Management
 * 
 * Physical Memory Manager (PMM) and Paging subsystem.
 * 
 * @file paging.h
 * @author Bengal Tiger OS (BengalEmpire)
 * @version 0.3.0
 */

#ifndef PAGING_H
#define PAGING_H

#include "common.h"

/* Page size: 4KB */
#define PAGE_SIZE 4096

/* Page flags */
#define PAGE_PRESENT    0x01
#define PAGE_WRITABLE   0x02
#define PAGE_USER       0x04
#define PAGE_ACCESSED   0x20
#define PAGE_DIRTY      0x40

/* ============================================ */
/* Physical Memory Manager                      */
/* ============================================ */

/**
 * Initialize the physical memory manager.
 * Creates a bitmap to track allocated page frames.
 * 
 * @param mem_size Total system memory in bytes
 * @param kernel_end Address of end of kernel (from linker)
 */
void pmm_init(uint32_t mem_size, uint32_t kernel_end);

/**
 * Allocate a physical page frame.
 * Uses first-fit search in bitmap.
 * 
 * @return Physical address of 4KB frame, or 0 if out of memory
 */
uint32_t pmm_alloc_frame(void);

/**
 * Free a physical page frame.
 * 
 * @param addr Physical address to free (must be page-aligned)
 */
void pmm_free_frame(uint32_t addr);

/**
 * Get total number of frames.
 */
uint32_t pmm_get_total_frames(void);

/**
 * Get number of used frames.
 */
uint32_t pmm_get_used_frames(void);

/* ============================================ */
/* Virtual Memory / Paging                      */
/* ============================================ */

/**
 * Enable paging and set up initial page directory.
 * Identity maps the first 4MB by default.
 * 
 * @param mem_size Total system memory in bytes
 */
void paging_install(uint32_t mem_size);

/**
 * Map a virtual address to a physical address.
 * 
 * @param virt Virtual address to map
 * @param phys Physical address destination
 * @param flags Page flags (PAGE_PRESENT | PAGE_WRITABLE | etc.)
 */
void paging_map(uint32_t virt, uint32_t phys, uint32_t flags);

/**
 * Unmap a virtual address.
 * 
 * @param virt Virtual address to unmap
 */
void paging_unmap(uint32_t virt);

/**
 * Get physical address for a virtual address.
 * 
 * @param virt Virtual address
 * @return Physical address, or 0 if not mapped
 */
uint32_t paging_get_physical(uint32_t virt);

/**
 * Flush TLB entry for a specific address.
 * 
 * @param addr Address to invalidate
 */
static inline void paging_invalidate(uint32_t addr) {
    __asm__ volatile("invlpg (%0)" : : "r"(addr) : "memory");
}

/**
 * Flush entire TLB (reload CR3).
 */
void paging_flush_tlb(void);

#endif /* PAGING_H */