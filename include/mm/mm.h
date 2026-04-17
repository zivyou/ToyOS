//
// Created by ziv on 2022/3/31.
//

#ifndef TOYOS_MM_H
#define TOYOS_MM_H

#include "types.h"

#define PAGE_SIZE (0x1000)
#define PAGE_SHIFT 12

// Physical memory bitmap
#define BITMAP_SIZE 1024  // Max 4MB of bitmap can track 4GB memory
extern uint32_t g_pmm_bitmap[BITMAP_SIZE];

typedef uint32_t page_table_entry_t;
typedef uint32_t page_directory_entry_t;

static page_directory_entry_t page_directories[1024] __attribute__((aligned(PAGE_SIZE)));
static page_table_entry_t page_tables[PAGE_SIZE/sizeof(page_table_entry_t)] __attribute__((aligned(PAGE_SIZE)));

// Physical memory manager functions
void pmm_init();
uint32_t pmm_alloc_page();
void pmm_free_page(uint32_t page_addr);
void pmm_mark_region(uint32_t start, uint32_t end, int used);

void mm_init();

#endif //TOYOS_MM_H
