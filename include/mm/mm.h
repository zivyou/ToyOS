//
// Created by ziv on 2022/3/31.
//

#ifndef TOYOS_MM_H
#define TOYOS_MM_H

#include "types.h"

#define PAGE_SIZE (0x1000)

typedef uint32_t page_table_entry_t;

typedef uint32_t page_directory_entry_t;

static page_directory_entry_t page_directories[1024] __attribute__((aligned(PAGE_SIZE)));
static page_table_entry_t page_tables[PAGE_SIZE/sizeof(page_table_entry_t)] __attribute__((aligned(PAGE_SIZE)));


void mm_init();

#endif //TOYOS_MM_H
