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

// Allocate a single page
// Returns: physical address of allocated page, 0 on failure
uint32_t pmm_alloc_page();

// Free a single page
// page_addr: physical address of the page to free
void pmm_free_page(uint32_t page_addr);

// Allocate multiple contiguous pages
// page_count: number of pages to allocate
// Returns: physical address of first allocated page, 0 on failure
uint32_t pmm_alloc_pages(uint32_t page_count);

// Free multiple contiguous pages
// page_addr: physical address of the first page
// page_count: number of pages to free
void pmm_free_pages(uint32_t page_addr, uint32_t page_count);

// Get memory statistics
uint32_t pmm_get_free_pages();
uint32_t pmm_get_total_pages();

// Mark a memory region as used or free (internal use)
void pmm_mark_region(uint32_t start, uint32_t end, int used);

// Print memory statistics
void pmm_print_stats();

void mm_init();

#endif //TOYOS_MM_H
