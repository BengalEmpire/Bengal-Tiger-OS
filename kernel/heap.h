/**
 * Bengal Tiger OS - Interrupt-Safe Expandable Kernel Heap Allocator
 *
 * First-fit free-list allocator with immediate bidirectional coalescing,
 * corruption detection, aligned allocations, and dynamic expansion up to 16MB.
 *
 * @file heap.h
 * @author Bengal Tiger OS (BengalEmpire)
 * @version 1.0.0
 */

#ifndef HEAP_H
#define HEAP_H

#include "common.h"

/* Heap Configuration (inside 32MB identity-mapped kernel space) */
#define HEAP_START          0x00400000U  /* 4 MB */
#define HEAP_INITIAL_SIZE   0x00200000U  /* 2 MB initial */
#define HEAP_MAX_SIZE       0x01000000U  /* 16 MB max (up to 20 MB) */
#define HEAP_BLOCK_MAGIC    0xDEADBEEFU
#define HEAP_ALIGNED_MAGIC  0xA116AED0U

typedef struct heap_block {
    uint32_t magic;
    uint32_t size;
    uint8_t  is_free;
    struct heap_block *next;
    struct heap_block *prev;
} heap_block_t;

typedef struct {
    uint32_t total_size;
    uint32_t used_size;
    uint32_t free_size;
    uint32_t num_allocations;
    uint32_t num_blocks;
} heap_stats_t;

extern heap_stats_t heap_stats;

void heap_init(void);
void heap_init_at(void *base, uint32_t initial_size, uint32_t max_size);
void *kmalloc(uint32_t size);
void *kzalloc(uint32_t size);
void *kmalloc_aligned(uint32_t size, uint32_t alignment);
void kfree(void *ptr);
void *krealloc(void *ptr, uint32_t new_size);
void heap_get_stats(heap_stats_t *stats);
int heap_check(void);

#endif /* HEAP_H */
