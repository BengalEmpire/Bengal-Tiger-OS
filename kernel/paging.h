#ifndef PAGING_H
#define PAGING_H

#include "common.h"

#define PAGE_SIZE 4096

void pmm_init(uint32_t mem_size, uint32_t kernel_end);
uint32_t pmm_alloc_frame();
void pmm_free_frame(uint32_t addr);
void paging_install(uint32_t mem_size);

#endif