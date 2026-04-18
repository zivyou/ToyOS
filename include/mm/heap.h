//
// Created by ziv on 2022/4/1.
//

#ifndef TOYOS_HEAP_H
#define TOYOS_HEAP_H

#include "types.h"

// Kernel heap virtual address space
#define KERNEL_HEAP_START 0xD0000000 // 3G 256M
#define KERNEL_HEAP_END   0xE0000000 // 3G 512M
#define KERNEL_HEAP_SIZE  (KERNEL_HEAP_END - KERNEL_HEAP_START)

// Heap block header
typedef struct heap_block {
    uint32_t size;        // Size of this block (including header)
    uint32_t used;        // 1 if used, 0 if free
    struct heap_block* next;  // Next block in free list (only valid for free blocks)
    struct heap_block* prev;  // Previous block in free list (only valid for free blocks)
} heap_block_t;

// Initialize kernel heap
void heap_init();

// Allocate memory from kernel heap
// size: number of bytes to allocate
// Returns: pointer to allocated memory, 0 on failure
void* kmalloc(uint32_t size);

// Free memory allocated with kmalloc
// ptr: pointer to memory to free
void kfree(void* ptr);

// Get heap statistics
uint32_t heap_get_total_size();
uint32_t heap_get_used_size();
uint32_t heap_get_free_size();
uint32_t heap_get_total_blocks();
uint32_t heap_get_used_blocks();
uint32_t heap_get_free_blocks();

// Print heap statistics and dump heap blocks
void heap_print_stats();

#endif //TOYOS_HEAP_H
