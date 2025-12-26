/**
 * Bengal Tiger OS - Kernel Heap Allocator
 * 
 * Provides dynamic memory allocation services for the kernel.
 * Uses a simple first-fit free-list allocator with block coalescing.
 * 
 * Features:
 * - kmalloc() - Allocate memory
 * - kfree() - Free memory
 * - krealloc() - Resize allocation
 * - Memory usage statistics
 * 
 * @file heap.h
 * @author Bengal Tiger OS (BengalEmpire)
 * @version 0.3.0
 */

#ifndef HEAP_H
#define HEAP_H

#include "common.h"

/* Heap Configuration */
#define HEAP_START          0x00400000  /* Start at 4MB mark */
#define HEAP_INITIAL_SIZE   0x00100000  /* 1MB initial heap */
#define HEAP_MAX_SIZE       0x01000000  /* 16MB maximum heap */
#define HEAP_BLOCK_MAGIC    0xDEADBEEF  /* Magic number for validity check */

/* Block Header Structure */
typedef struct heap_block {
    uint32_t magic;             /* Magic number for corruption detection */
    uint32_t size;              /* Size of data area (excluding header) */
    uint8_t is_free;            /* 1 if free, 0 if allocated */
    struct heap_block *next;    /* Next block in list */
    struct heap_block *prev;    /* Previous block in list */
} heap_block_t;

/* Heap Statistics */
typedef struct {
    uint32_t total_size;        /* Total heap size */
    uint32_t used_size;         /* Currently allocated */
    uint32_t free_size;         /* Currently free */
    uint32_t num_allocations;   /* Number of active allocations */
    uint32_t num_blocks;        /* Total number of blocks */
} heap_stats_t;

/* Global Heap Statistics */
extern heap_stats_t heap_stats;

/**
 * Initialize the kernel heap.
 * Sets up the initial heap region and free list.
 */
void heap_init(void);

/**
 * Allocate memory from the kernel heap.
 * @param size Number of bytes to allocate
 * @return Pointer to allocated memory, or NULL if failed
 */
void *kmalloc(uint32_t size);

/**
 * Allocate zeroed memory from the kernel heap.
 * @param size Number of bytes to allocate
 * @return Pointer to zeroed memory, or NULL if failed
 */
void *kzalloc(uint32_t size);

/**
 * Allocate aligned memory from the kernel heap.
 * @param size Number of bytes to allocate
 * @param alignment Alignment requirement (must be power of 2)
 * @return Pointer to aligned memory, or NULL if failed
 */
void *kmalloc_aligned(uint32_t size, uint32_t alignment);

/**
 * Free previously allocated memory.
 * @param ptr Pointer to memory to free (NULL is safe)
 */
void kfree(void *ptr);

/**
 * Resize an allocation.
 * @param ptr Pointer to existing allocation (NULL = kmalloc)
 * @param new_size New size in bytes (0 = kfree)
 * @return Pointer to resized memory, or NULL if failed
 */
void *krealloc(void *ptr, uint32_t new_size);

/**
 * Get current heap statistics.
 * @param stats Pointer to stats structure to fill
 */
void heap_get_stats(heap_stats_t *stats);

/**
 * Check heap integrity.
 * @return 1 if heap is valid, 0 if corrupted
 */
int heap_check(void);

#endif /* HEAP_H */
