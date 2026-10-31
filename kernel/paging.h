/**
 * Bengal Tiger OS - Physical Memory Manager (PMM) & Virtual Paging
 *
 * @file paging.h
 * @author Bengal Tiger OS (BengalEmpire)
 * @version 1.0.0
 */

#ifndef PAGING_H
#define PAGING_H

#include "common.h"
#include "multiboot.h"

#define PAGE_SIZE       4096

/* x86 Page Directory / Page Table Entry Flags */
#define PAGE_PRESENT    0x001
#define PAGE_WRITABLE   0x002
#define PAGE_USER       0x004
#define PAGE_PWT        0x008   /* Page-level Write-Through (for MMIO) */
#define PAGE_PCD        0x010   /* Page-level Cache Disable (for MMIO) */
#define PAGE_ACCESSED   0x020
#define PAGE_DIRTY      0x040
#define PAGE_MMIO       (PAGE_PRESENT | PAGE_WRITABLE | PAGE_PWT | PAGE_PCD)

/* Identity-mapped low memory window: 32 MB (0x00000000 - 0x01FFFFFF) */
#define IDENTITY_MAP_TABLES 8
#define IDENTITY_MAP_SIZE   (IDENTITY_MAP_TABLES * 4 * 1024 * 1024)

/* ============================================ */
/* Physical Memory Manager                      */
/* ============================================ */

void pmm_init(uint32_t mem_size, uint32_t reserved_end);
void pmm_init_mmap(multiboot_info_t *mbi, uint32_t fallback_mem_size, uint32_t reserved_end);
void pmm_reserve_region(uint32_t base, uint32_t size);
uint32_t pmm_alloc_frame(void);
void pmm_free_frame(uint32_t addr);
uint32_t pmm_get_total_frames(void);
uint32_t pmm_get_used_frames(void);
uint32_t pmm_get_free_frames(void);

/* ============================================ */
/* Virtual Memory / Paging                      */
/* ============================================ */

void paging_install(uint32_t mem_size);
void paging_map(uint32_t virt, uint32_t phys, uint32_t flags);
void paging_map_region(uint32_t virt, uint32_t phys, uint32_t size, uint32_t flags);
void paging_unmap(uint32_t virt);
uint32_t paging_get_physical(uint32_t virt);

static inline void paging_invalidate(uint32_t addr) {
    __asm__ volatile("invlpg (%0)" : : "r"(addr) : "memory");
}

void paging_flush_tlb(void);

#endif /* PAGING_H */
