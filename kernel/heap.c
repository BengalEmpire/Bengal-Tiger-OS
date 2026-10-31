/**
 * Bengal Tiger OS - Interrupt-Safe Expandable Kernel Heap Implementation
 *
 * @file heap.c
 * @author Bengal Tiger OS (BengalEmpire)
 * @version 1.0.0
 */

#include "heap.h"
#include "common.h"
#ifdef __i386__
#include "paging.h"
#endif

heap_stats_t heap_stats = {0, 0, 0, 0, 0};

static heap_block_t *heap_head = NULL;
static uintptr_t heap_base_addr = HEAP_START;
static uint32_t heap_cur_size   = 0;
static uint32_t heap_max_limit  = HEAP_MAX_SIZE;

#define MIN_BLOCK_SIZE 16
#define HEADER_SIZE    ((uint32_t)((sizeof(heap_block_t) + 7) & ~7))

void heap_init_at(void *base, uint32_t initial_size, uint32_t max_size) {
    uint32_t irq = irq_save();

    heap_base_addr = (uintptr_t)base;
    heap_cur_size  = initial_size;
    heap_max_limit = max_size;

    heap_head = (heap_block_t *)heap_base_addr;
    heap_head->magic   = HEAP_BLOCK_MAGIC;
    heap_head->size    = initial_size - HEADER_SIZE;
    heap_head->is_free = 1;
    heap_head->next    = NULL;
    heap_head->prev    = NULL;

    heap_stats.total_size      = initial_size;
    heap_stats.free_size       = heap_head->size;
    heap_stats.used_size       = 0;
    heap_stats.num_allocations = 0;
    heap_stats.num_blocks      = 1;

    irq_restore(irq);
}

void heap_init(void) {
    heap_init_at((void *)(uintptr_t)HEAP_START, HEAP_INITIAL_SIZE, HEAP_MAX_SIZE);
}

static heap_block_t *find_free_block(uint32_t size) {
    heap_block_t *current = heap_head;
    while (current != NULL) {
        if (current->is_free && current->size >= size) {
            return current;
        }
        current = current->next;
    }
    return NULL;
}

static void split_block(heap_block_t *block, uint32_t size) {
    if (block->size < size + HEADER_SIZE + MIN_BLOCK_SIZE) {
        return;
    }

    uint32_t remaining = block->size - size - HEADER_SIZE;
    heap_block_t *new_block = (heap_block_t *)((uint8_t *)block + HEADER_SIZE + size);

    new_block->magic   = HEAP_BLOCK_MAGIC;
    new_block->size    = remaining;
    new_block->is_free = 1;
    new_block->prev    = block;
    new_block->next    = block->next;

    if (block->next != NULL) {
        block->next->prev = new_block;
    }

    block->next = new_block;
    block->size = size;

    heap_stats.num_blocks++;
    heap_stats.free_size -= HEADER_SIZE;
}

static void coalesce(heap_block_t *block) {
    if (block->next != NULL && block->next->is_free) {
        block->size += HEADER_SIZE + block->next->size;
        block->next = block->next->next;
        if (block->next != NULL) {
            block->next->prev = block;
        }
        heap_stats.num_blocks--;
        heap_stats.free_size += HEADER_SIZE;
    }

    if (block->prev != NULL && block->prev->is_free) {
        block->prev->size += HEADER_SIZE + block->size;
        block->prev->next = block->next;
        if (block->next != NULL) {
            block->next->prev = block->prev;
        }
        heap_stats.num_blocks--;
        heap_stats.free_size += HEADER_SIZE;
    }
}

static int heap_expand(uint32_t min_bytes) {
    uint32_t grow = ALIGN_UP(min_bytes + HEADER_SIZE, 0x100000U); /* Grow in 1MB chunks */
    if (heap_cur_size + grow > heap_max_limit) {
        if (heap_cur_size >= heap_max_limit) return 0;
        grow = heap_max_limit - heap_cur_size;
        if (grow < min_bytes + HEADER_SIZE) return 0;
    }

#ifdef __i386__
    pmm_reserve_region((uint32_t)(heap_base_addr + heap_cur_size), grow);
#endif

    heap_block_t *new_blk = (heap_block_t *)(heap_base_addr + heap_cur_size);
    new_blk->magic   = HEAP_BLOCK_MAGIC;
    new_blk->size    = grow - HEADER_SIZE;
    new_blk->is_free = 1;
    new_blk->next    = NULL;

    heap_block_t *tail = heap_head;
    while (tail->next != NULL) {
        tail = tail->next;
    }
    tail->next    = new_blk;
    new_blk->prev = tail;

    heap_cur_size         += grow;
    heap_stats.total_size += grow;
    heap_stats.free_size  += new_blk->size;
    heap_stats.num_blocks++;

    coalesce(new_blk);
    return 1;
}

void *kmalloc(uint32_t size) {
    if (size == 0 || !heap_head) {
        return NULL;
    }

    size = (size + 7U) & ~7U;

    uint32_t irq = irq_save();

    heap_block_t *block = find_free_block(size);
    if (block == NULL) {
        if (heap_expand(size)) {
            block = find_free_block(size);
        }
    }

    if (block == NULL) {
        irq_restore(irq);
        return NULL;
    }

    split_block(block, size);
    block->is_free = 0;

    heap_stats.used_size += block->size;
    heap_stats.free_size -= block->size;
    heap_stats.num_allocations++;

    void *res = (void *)((uint8_t *)block + HEADER_SIZE);
    irq_restore(irq);
    return res;
}

void *kzalloc(uint32_t size) {
    void *ptr = kmalloc(size);
    if (ptr != NULL) {
        memset(ptr, 0, size);
    }
    return ptr;
}

void *kmalloc_aligned(uint32_t size, uint32_t alignment) {
    if (alignment < 8) alignment = 8;
    if (!IS_POWER_OF_2(alignment)) return NULL;

    uint32_t meta = 2 * sizeof(uintptr_t);
    uint32_t total_size = size + alignment + meta;
    void *raw_ptr = kmalloc(total_size);
    if (raw_ptr == NULL) return NULL;

    uintptr_t addr = (uintptr_t)raw_ptr + meta;
    uintptr_t aligned_addr = (addr + (uintptr_t)alignment - 1) & ~((uintptr_t)alignment - 1);

    uintptr_t *slot = (uintptr_t *)aligned_addr;
    slot[-1] = HEAP_ALIGNED_MAGIC;
    slot[-2] = (uintptr_t)raw_ptr;

    return (void *)aligned_addr;
}

void kfree(void *ptr) {
    if (ptr == NULL || !heap_head) {
        return;
    }

    uint32_t irq = irq_save();

    uintptr_t p = (uintptr_t)ptr;
    if (p < heap_base_addr + HEADER_SIZE || p >= heap_base_addr + heap_cur_size) {
        irq_restore(irq);
        return;
    }

    heap_block_t *block = (heap_block_t *)((uint8_t *)ptr - HEADER_SIZE);

    if (block->magic != HEAP_BLOCK_MAGIC) {
        uintptr_t *slot = (uintptr_t *)ptr;
        if (p >= heap_base_addr + HEADER_SIZE + 2 * sizeof(uintptr_t) &&
            slot[-1] == HEAP_ALIGNED_MAGIC) {
            void *raw_ptr = (void *)slot[-2];
            if ((uintptr_t)raw_ptr >= heap_base_addr + HEADER_SIZE &&
                (uintptr_t)raw_ptr < heap_base_addr + heap_cur_size) {
                block = (heap_block_t *)((uint8_t *)raw_ptr - HEADER_SIZE);
            }
        }
    }

    if (block->magic != HEAP_BLOCK_MAGIC || block->is_free) {
        irq_restore(irq);
        return;
    }

    block->is_free = 1;

    heap_stats.used_size -= block->size;
    heap_stats.free_size += block->size;
    if (heap_stats.num_allocations > 0) {
        heap_stats.num_allocations--;
    }

    coalesce(block);
    irq_restore(irq);
}

void *krealloc(void *ptr, uint32_t new_size) {
    if (ptr == NULL) return kmalloc(new_size);
    if (new_size == 0) {
        kfree(ptr);
        return NULL;
    }

    heap_block_t *block = (heap_block_t *)((uint8_t *)ptr - HEADER_SIZE);
    if (block->magic != HEAP_BLOCK_MAGIC) return NULL;
    if (block->size >= new_size) return ptr;

    void *new_ptr = kmalloc(new_size);
    if (new_ptr != NULL) {
        memcpy(new_ptr, ptr, block->size);
        kfree(ptr);
    }
    return new_ptr;
}

void heap_get_stats(heap_stats_t *stats) {
    if (stats != NULL) {
        uint32_t irq = irq_save();
        *stats = heap_stats;
        irq_restore(irq);
    }
}

int heap_check(void) {
    uint32_t irq = irq_save();
    heap_block_t *current = heap_head;
    while (current != NULL) {
        if (current->magic != HEAP_BLOCK_MAGIC) {
            irq_restore(irq);
            return 0;
        }
        current = current->next;
    }
    irq_restore(irq);
    return 1;
}
