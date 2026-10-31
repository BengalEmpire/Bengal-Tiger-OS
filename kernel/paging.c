/**
 * Bengal Tiger OS - Memory Management Implementation
 *
 * Bitmap Physical Memory Manager (E820-aware, 4GB-safe) and 32-bit x86 Paging
 * with 32MB identity mapping and safe high-MMIO mapping (VBE, E1000, etc.).
 *
 * @file paging.c
 * @author Bengal Tiger OS (BengalEmpire)
 * @version 1.0.0
 */

#include "paging.h"
#include "heap.h"
#include "common.h"

/* ============================================ */
/* Physical Memory Manager                      */
/* ============================================ */

#define BITMAP_SIZE 32768   /* 32768 * 32 = 1,048,576 frames = 4GB */
static uint32_t bitmap[BITMAP_SIZE];

static uint32_t total_frames = 0;
static uint32_t used_frames  = 0;

/* Static fallback pool of 32 page-aligned page tables (128KB) in BSS so
 * mapping high MMIO (VBE framebuffer, PCI BARs) is ALWAYS virtual-safe
 * even on low-memory systems. */
#define STATIC_PT_POOL_COUNT 32
static uint32_t static_pt_pool[STATIC_PT_POOL_COUNT][1024] __attribute__((aligned(4096)));
static uint32_t static_pt_used = 0;

static inline void frame_set_used(uint32_t frame) {
    if (frame / 32 < BITMAP_SIZE) {
        uint32_t mask = (1U << (frame % 32));
        if (!(bitmap[frame / 32] & mask)) {
            bitmap[frame / 32] |= mask;
            used_frames++;
        }
    }
}

static inline void frame_set_free(uint32_t frame) {
    if (frame / 32 < BITMAP_SIZE) {
        uint32_t mask = (1U << (frame % 32));
        if (bitmap[frame / 32] & mask) {
            bitmap[frame / 32] &= ~mask;
            if (used_frames > 0) used_frames--;
        }
    }
}

void pmm_reserve_region(uint32_t base, uint32_t size) {
    uint32_t start_frame = ALIGN_DOWN(base, PAGE_SIZE) / PAGE_SIZE;
    uint64_t end_addr = (uint64_t)base + (uint64_t)size;
    if (end_addr > 0xFFFFF000ULL) end_addr = 0xFFFFF000ULL;
    uint32_t end_frame = (uint32_t)(ALIGN_UP((uint32_t)end_addr, PAGE_SIZE) / PAGE_SIZE);

    for (uint32_t f = start_frame; f < end_frame && f < BITMAP_SIZE * 32; f++) {
        frame_set_used(f);
    }
}

void pmm_init(uint32_t mem_size, uint32_t reserved_end) {
    total_frames = mem_size / PAGE_SIZE;
    if (total_frames > BITMAP_SIZE * 32) {
        total_frames = BITMAP_SIZE * 32;
    }

    memset(bitmap, 0xFF, sizeof(bitmap));
    used_frames = total_frames;

    /* Protect low memory, kernel, initial page tables, and entire max heap */
    uint32_t min_free = ALIGN_UP(reserved_end, PAGE_SIZE);
    uint32_t heap_limit = HEAP_START + HEAP_MAX_SIZE;
    if (min_free < heap_limit) {
        min_free = heap_limit;
    }

    for (uint32_t f = min_free / PAGE_SIZE; f < total_frames; f++) {
        frame_set_free(f);
    }
}

void pmm_init_mmap(multiboot_info_t *mbi, uint32_t fallback_mem_size, uint32_t reserved_end) {
    if (!mbi || !(mbi->flags & (1 << 6)) || mbi->mmap_length == 0) {
        pmm_init(fallback_mem_size, reserved_end);
        return;
    }

    memset(bitmap, 0xFF, sizeof(bitmap));
    total_frames = 0;
    used_frames  = 0;

    uint64_t highest_avail = 0;
    multiboot_mmap_entry_t *entry = multiboot_mmap_first(mbi);
    while (entry && multiboot_mmap_has_more(mbi, entry)) {
        if (entry->type == MULTIBOOT_MMAP_AVAILABLE && entry->addr < 0xFFFFF000ULL) {
            uint64_t start = (entry->addr + PAGE_SIZE - 1) & ~(uint64_t)(PAGE_SIZE - 1);
            uint64_t end = entry->addr + entry->len;
            if (end > 0xFFFFF000ULL) end = 0xFFFFF000ULL;
            end &= ~(uint64_t)(PAGE_SIZE - 1);
            if (end > start && end > highest_avail) {
                highest_avail = end;
            }
        }
        entry = multiboot_mmap_next(entry);
    }

    if (highest_avail == 0) {
        pmm_init(fallback_mem_size, reserved_end);
        return;
    }

    total_frames = (uint32_t)(highest_avail / PAGE_SIZE);
    used_frames  = total_frames;

    /* Free pages strictly within E820 AVAILABLE regions */
    entry = multiboot_mmap_first(mbi);
    while (entry && multiboot_mmap_has_more(mbi, entry)) {
        if (entry->type == MULTIBOOT_MMAP_AVAILABLE && entry->addr < 0xFFFFF000ULL) {
            uint64_t start = (entry->addr + PAGE_SIZE - 1) & ~(uint64_t)(PAGE_SIZE - 1);
            uint64_t end = entry->addr + entry->len;
            if (end > 0xFFFFF000ULL) end = 0xFFFFF000ULL;
            end &= ~(uint64_t)(PAGE_SIZE - 1);

            for (uint64_t addr = start; addr < end; addr += PAGE_SIZE) {
                frame_set_free((uint32_t)(addr / PAGE_SIZE));
            }
        }
        entry = multiboot_mmap_next(entry);
    }

    /* Always reserve first 1MB (BIOS/VGA), kernel + page tables, and kernel heap */
    uint32_t low_protect = ALIGN_UP(reserved_end, PAGE_SIZE);
    uint32_t heap_limit  = HEAP_START + HEAP_MAX_SIZE;
    if (low_protect < heap_limit) {
        low_protect = heap_limit;
    }
    pmm_reserve_region(0, low_protect);
}

uint32_t pmm_alloc_frame(void) {
    uint32_t irq = irq_save();
    for (uint32_t i = 0; i < BITMAP_SIZE; i++) {
        if (bitmap[i] != 0xFFFFFFFFU) {
            for (uint32_t j = 0; j < 32; j++) {
                if (!(bitmap[i] & (1U << j))) {
                    uint32_t frame = i * 32 + j;
                    if (frame >= total_frames) {
                        irq_restore(irq);
                        return 0;
                    }
                    bitmap[i] |= (1U << j);
                    used_frames++;
                    irq_restore(irq);
                    return frame * PAGE_SIZE;
                }
            }
        }
    }
    irq_restore(irq);
    return 0;
}

void pmm_free_frame(uint32_t addr) {
    uint32_t irq = irq_save();
    uint32_t frame = addr / PAGE_SIZE;
    if (frame < total_frames) {
        frame_set_free(frame);
    }
    irq_restore(irq);
}

uint32_t pmm_get_total_frames(void) {
    return total_frames;
}

uint32_t pmm_get_used_frames(void) {
    return used_frames;
}

uint32_t pmm_get_free_frames(void) {
    return (total_frames >= used_frames) ? (total_frames - used_frames) : 0;
}

/* ============================================ */
/* Paging Setup                                 */
/* ============================================ */

static uint32_t *page_dir = NULL;

static uint32_t* alloc_page_table(void) {
    /* Prefer static identity-mapped pool first so pt is guaranteed mapped */
    if (static_pt_used < STATIC_PT_POOL_COUNT) {
        uint32_t *pt = static_pt_pool[static_pt_used++];
        memset(pt, 0, PAGE_SIZE);
        return pt;
    }

    /* Otherwise allocate a physical frame within the 32MB identity-mapped window */
    uint32_t phys = pmm_alloc_frame();
    if (phys != 0 && phys < IDENTITY_MAP_SIZE) {
        uint32_t *pt = (uint32_t*)phys;
        memset(pt, 0, PAGE_SIZE);
        return pt;
    }
    if (phys != 0) {
        pmm_free_frame(phys);
    }
    return NULL;
}

void paging_install(uint32_t mem_size) {
    UNUSED(mem_size);

    extern uint32_t bss_end;
    uint32_t kernel_end = ALIGN_UP((uint32_t)&bss_end, PAGE_SIZE);

    page_dir = (uint32_t*)kernel_end;
    uint32_t *pts = (uint32_t*)(kernel_end + PAGE_SIZE);

    memset(page_dir, 0, PAGE_SIZE);

    /* Identity-map first 32MB (8 page tables x 4MB) covering:
     * BIOS/VGA (0-1MB), Kernel + BSS + Page Tables (1-4MB),
     * Expandable 16MB Kernel Heap (4-20MB), and low PMM frames (20-32MB). */
    for (uint32_t pt_idx = 0; pt_idx < IDENTITY_MAP_TABLES; pt_idx++) {
        uint32_t *pt = pts + (pt_idx * 1024);
        page_dir[pt_idx] = (uint32_t)pt | PAGE_PRESENT | PAGE_WRITABLE;

        for (uint32_t pg_idx = 0; pg_idx < 1024; pg_idx++) {
            uint32_t addr = (pt_idx * 4 * 1024 * 1024) + (pg_idx * PAGE_SIZE);
            pt[pg_idx] = addr | PAGE_PRESENT | PAGE_WRITABLE;
        }
    }

    for (uint32_t i = IDENTITY_MAP_TABLES; i < 1024; i++) {
        page_dir[i] = PAGE_WRITABLE;
    }

    __asm__ volatile("mov %0, %%cr3" : : "r"(page_dir) : "memory");

    uint32_t cr0;
    __asm__ volatile("mov %%cr0, %0" : "=r"(cr0));
    cr0 |= 0x80000000U;
    __asm__ volatile("mov %0, %%cr0" : : "r"(cr0) : "memory");
}

void paging_map(uint32_t virt, uint32_t phys, uint32_t flags) {
    if (!page_dir) return;

    uint32_t pd_index = virt >> 22;
    uint32_t pt_index = (virt >> 12) & 0x3FF;

    uint32_t *pt;
    if (page_dir[pd_index] & PAGE_PRESENT) {
        pt = (uint32_t*)(page_dir[pd_index] & ~0xFFFU);
    } else {
        pt = alloc_page_table();
        if (!pt) return;
        page_dir[pd_index] = (uint32_t)pt | PAGE_PRESENT | PAGE_WRITABLE | (flags & (PAGE_USER | PAGE_PWT | PAGE_PCD));
    }

    pt[pt_index] = (phys & ~0xFFFU) | (flags | PAGE_PRESENT);
    paging_invalidate(virt);
}

void paging_map_region(uint32_t virt, uint32_t phys, uint32_t size, uint32_t flags) {
    if (size == 0) return;
    uint32_t offset = phys & (PAGE_SIZE - 1);
    uint32_t v_base = ALIGN_DOWN(virt, PAGE_SIZE);
    uint32_t p_base = ALIGN_DOWN(phys, PAGE_SIZE);
    uint32_t total  = ALIGN_UP(size + offset, PAGE_SIZE);

    for (uint32_t off = 0; off < total; off += PAGE_SIZE) {
        paging_map(v_base + off, p_base + off, flags);
    }
}

void paging_unmap(uint32_t virt) {
    if (!page_dir) return;

    uint32_t pd_index = virt >> 22;
    uint32_t pt_index = (virt >> 12) & 0x3FF;

    if (!(page_dir[pd_index] & PAGE_PRESENT)) return;

    uint32_t *pt = (uint32_t*)(page_dir[pd_index] & ~0xFFFU);
    pt[pt_index] = 0;
    paging_invalidate(virt);
}

uint32_t paging_get_physical(uint32_t virt) {
    if (!page_dir) return virt;

    uint32_t pd_index = virt >> 22;
    uint32_t pt_index = (virt >> 12) & 0x3FF;

    if (!(page_dir[pd_index] & PAGE_PRESENT)) return 0;

    uint32_t *pt = (uint32_t*)(page_dir[pd_index] & ~0xFFFU);
    if (!(pt[pt_index] & PAGE_PRESENT)) return 0;

    return (pt[pt_index] & ~0xFFFU) | (virt & 0xFFFU);
}

void paging_flush_tlb(void) {
    uint32_t cr3;
    __asm__ volatile("mov %%cr3, %0" : "=r"(cr3));
    __asm__ volatile("mov %0, %%cr3" : : "r"(cr3) : "memory");
}
