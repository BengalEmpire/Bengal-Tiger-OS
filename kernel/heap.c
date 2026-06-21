/**
 * Bengal Tiger OS - Kernel Heap Allocator Implementation
 * 
 * First-fit free-list allocator with immediate coalescing.
 * 
 * @file heap.c
 * @author Bengal Tiger OS (BengalEmpire)
 * @version 0.3.0
 */

#include "heap.h"
#include "common.h"

/* Global heap statistics */
heap_stats_t heap_stats = {0, 0, 0, 0, 0};

/* Head of free list */
static heap_block_t *heap_head = NULL;

/* Heap bounds */
static uint32_t heap_start = HEAP_START;

/* Minimum block size to prevent excessive fragmentation */
#define MIN_BLOCK_SIZE 16

/* Header size aligned to 8 bytes */
#define HEADER_SIZE ((sizeof(heap_block_t) + 7) & ~7)

void heap_init(void) {
    /* Initialize the heap as one large free block */
    heap_head = (heap_block_t *)heap_start;
    
    heap_head->magic = HEAP_BLOCK_MAGIC;
    heap_head->size = HEAP_INITIAL_SIZE - HEADER_SIZE;
    heap_head->is_free = 1;
    heap_head->next = NULL;
    heap_head->prev = NULL;

    /* Initialize statistics */
    heap_stats.total_size = HEAP_INITIAL_SIZE;
    heap_stats.free_size = heap_head->size;
    heap_stats.used_size = 0;
    heap_stats.num_allocations = 0;
    heap_stats.num_blocks = 1;
}

/**
 * Find a free block that can fit the requested size (first-fit).
 */
static heap_block_t *find_free_block(uint32_t size) {
    heap_block_t *current = heap_head;
    
    while (current != NULL) {
        if (current->is_free && current->size >= size) {
            return current;
        }
        current = current->next;
    }
    
    return NULL;  /* No suitable block found */
}

/**
 * Split a block if it's large enough to hold both the allocation and a new free block.
 */
static void split_block(heap_block_t *block, uint32_t size) {
    uint32_t remaining = block->size - size - HEADER_SIZE;
    
    /* Only split if remaining space is worth it */
    if (remaining >= MIN_BLOCK_SIZE + HEADER_SIZE) {
        heap_block_t *new_block = (heap_block_t *)((uint8_t *)block + HEADER_SIZE + size);
        
        new_block->magic = HEAP_BLOCK_MAGIC;
        new_block->size = remaining;
        new_block->is_free = 1;
        new_block->prev = block;
        new_block->next = block->next;
        
        if (block->next != NULL) {
            block->next->prev = new_block;
        }
        
        block->next = new_block;
        block->size = size;
        
        heap_stats.num_blocks++;
        heap_stats.free_size -= HEADER_SIZE;
    }
}

/**
 * Coalesce adjacent free blocks.
 */
static void coalesce(heap_block_t *block) {
    /* Coalesce with next block if free */
    if (block->next != NULL && block->next->is_free) {
        block->size += HEADER_SIZE + block->next->size;
        block->next = block->next->next;
        
        if (block->next != NULL) {
            block->next->prev = block;
        }
        
        heap_stats.num_blocks--;
        heap_stats.free_size += HEADER_SIZE;
    }
    
    /* Coalesce with previous block if free */
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

void *kmalloc(uint32_t size) {
    if (size == 0) {
        return NULL;
    }
    
    /* Align size to 8 bytes */
    size = (size + 7) & ~7;
    
    heap_block_t *block = find_free_block(size);
    
    if (block == NULL) {
        /* TODO: Expand heap by requesting more pages from PMM */
        return NULL;
    }
    
    /* Split block if necessary */
    split_block(block, size);
    
    block->is_free = 0;
    
    /* Update statistics */
    heap_stats.used_size += block->size;
    heap_stats.free_size -= block->size;
    heap_stats.num_allocations++;
    
    /* Return pointer to data area (after header) */
    return (void *)((uint8_t *)block + HEADER_SIZE);
}

void *kzalloc(uint32_t size) {
    void *ptr = kmalloc(size);
    if (ptr != NULL) {
        memset(ptr, 0, size);
    }
    return ptr;
}

void *kmalloc_aligned(uint32_t size, uint32_t alignment) {
    if (alignment < 4) alignment = 4;
    
    /* Allocate enough space for size + alignment + 4 bytes to store the original pointer */
    uint32_t total_size = size + alignment + 4;
    void *raw_ptr = kmalloc(total_size);
    if (raw_ptr == NULL) return NULL;
    
    /* Calculate aligned address, leaving at least 4 bytes before it for the raw pointer */
    uint32_t addr = (uint32_t)raw_ptr + 4;
    uint32_t aligned_addr = (addr + alignment - 1) & ~(alignment - 1);

    /* Store the raw pointer immediately before the aligned address */
    ((void **)aligned_addr)[-1] = raw_ptr;

    return (void *)aligned_addr;
}

void kfree(void *ptr) {
    if (ptr == NULL) {
        return;
    }

    /* Check if this pointer is aligned and has a stored raw pointer.
     * We need a way to distinguish between normal kmalloc and kmalloc_aligned.
     * Since we don't have a flag in the header yet, we check if the pointer
     * immediately follows a valid heap_block_t header. */
    
    heap_block_t *block = (heap_block_t *)((uint8_t *)ptr - HEADER_SIZE);
    
    /* If the magic doesn't match, it might be an aligned pointer */
    if (block->magic != HEAP_BLOCK_MAGIC) {
        /* Try to retrieve original pointer from kmalloc_aligned */
        void *raw_ptr = ((void **)ptr)[-1];

        /* Validate the raw pointer */
        if (raw_ptr != NULL && (uint32_t)raw_ptr >= HEAP_START) {
            heap_block_t *raw_block = (heap_block_t *)((uint8_t *)raw_ptr - HEADER_SIZE);
            if (raw_block->magic == HEAP_BLOCK_MAGIC) {
                ptr = raw_ptr;
                block = raw_block;
            } else {
                /* Not a valid heap block */
                return;
            }
        } else {
            return;
        }
    }

    /* Validate magic number */
    if (block->magic != HEAP_BLOCK_MAGIC) {
        /* Heap corruption detected! */
        return;
    }
    
    if (block->is_free) {
        /* Double free detected! */
        return;
    }
    
    block->is_free = 1;
    
    /* Update statistics */
    heap_stats.used_size -= block->size;
    heap_stats.free_size += block->size;
    heap_stats.num_allocations--;
    
    /* Coalesce with adjacent free blocks */
    coalesce(block);
}

void *krealloc(void *ptr, uint32_t new_size) {
    if (ptr == NULL) {
        return kmalloc(new_size);
    }
    
    if (new_size == 0) {
        kfree(ptr);
        return NULL;
    }
    
    /* Get current block */
    heap_block_t *block = (heap_block_t *)((uint8_t *)ptr - HEADER_SIZE);
    
    /* If current block is large enough, just return it */
    if (block->size >= new_size) {
        return ptr;
    }
    
    /* Allocate new block and copy data */
    void *new_ptr = kmalloc(new_size);
    if (new_ptr != NULL) {
        memcpy(new_ptr, ptr, block->size);
        kfree(ptr);
    }
    
    return new_ptr;
}

void heap_get_stats(heap_stats_t *stats) {
    if (stats != NULL) {
        *stats = heap_stats;
    }
}

int heap_check(void) {
    heap_block_t *current = heap_head;
    
    while (current != NULL) {
        if (current->magic != HEAP_BLOCK_MAGIC) {
            return 0;  /* Corruption detected */
        }
        current = current->next;
    }
    
    return 1;  /* Heap is valid */
}
