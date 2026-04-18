//
// Created by ziv on 2022/4/1.
//
#include "mm/heap.h"
#include "mm/mm.h"
#include "mm/paging.h"
#include "printk.h"

// Current heap end (grows upward)
static uint32_t g_heap_end = KERNEL_HEAP_START;

// Free list head (sorted by address for coalescing)
static heap_block_t* g_free_list = 0;

// Statistics
static uint32_t g_total_size = 0;
static uint32_t g_used_size = 0;
static uint32_t g_total_blocks = 0;
static uint32_t g_used_blocks = 0;

// Helper: align size to 4-byte boundary
static inline uint32_t align_size(uint32_t size) {
    return (size + 3) & ~3;
}

// Helper: get heap block from user pointer
static inline heap_block_t* ptr_to_block(void* ptr) {
    return (heap_block_t*)((uint32_t)ptr - sizeof(heap_block_t));
}

// Helper: get user pointer from heap block
static inline void* block_to_ptr(heap_block_t* block) {
    return (void*)((uint32_t)block + sizeof(heap_block_t));
}

// Helper: check if address is in heap space
static inline int is_heap_addr(uint32_t addr) {
    return (addr >= KERNEL_HEAP_START && addr < KERNEL_HEAP_END);
}

// Add block to free list
static void add_to_free_list(heap_block_t* block) {
    // Find position to insert (sorted by address)
    heap_block_t* prev = 0;
    heap_block_t* curr = g_free_list;

    while (curr && (uint32_t)curr < (uint32_t)block) {
        prev = curr;
        curr = curr->next;
    }

    // Insert into list
    block->prev = prev;
    block->next = curr;

    if (prev) {
        prev->next = block;
    } else {
        g_free_list = block;
    }

    if (curr) {
        curr->prev = block;
    }

    // Mark as free
    block->used = 0;

    // Update statistics
    g_used_blocks--;
    g_used_size -= (block->size - sizeof(heap_block_t));
}

// Remove block from free list
static void remove_from_free_list(heap_block_t* block) {
    if (block->prev) {
        block->prev->next = block->next;
    } else {
        g_free_list = block->next;
    }

    if (block->next) {
        block->next->prev = block->prev;
    }

    block->prev = 0;
    block->next = 0;

    // Mark as used
    block->used = 1;

    // Update statistics
    g_used_blocks++;
    g_used_size += (block->size - sizeof(heap_block_t));
}

// Coalesce adjacent free blocks
static void coalesce_blocks(heap_block_t* block) {
    // Try to coalesce with next block
    uint32_t next_addr = (uint32_t)block + block->size;

    // Only try to coalesce if next_addr is within the heap and not at heap end
    if (is_heap_addr(next_addr) && next_addr < g_heap_end) {
        heap_block_t* next = (heap_block_t*)next_addr;

        // Check if next block is free (valid block with used=0)
        if (!next->used) {
            // Remove next from free list
            remove_from_free_list(next);
            g_total_blocks--;

            // Merge sizes
            block->size += next->size;
        }
    }

    // Try to coalesce with previous block
    if (block->prev && (uint32_t)block->prev + block->prev->size == (uint32_t)block) {
        heap_block_t* prev = block->prev;

        // Remove current from free list
        remove_from_free_list(block);
        g_total_blocks--;

        // Merge sizes
        prev->size += block->size;
    }
}

// Expand heap by allocating new pages
static heap_block_t* expand_heap(uint32_t min_size) {
    // Calculate how many pages we need
    uint32_t needed = min_size + sizeof(heap_block_t);
    uint32_t page_count = (needed + PAGE_SIZE - 1) / PAGE_SIZE;

    // Allocate physical pages
    uint32_t phys_addr = pmm_alloc_pages(page_count);
    if (phys_addr == 0) {
        printk("ERROR: Failed to allocate %d pages for heap expansion\n", page_count);
        return 0;
    }

    // Map pages into heap space
    uint32_t virt_addr = g_heap_end;
    uint32_t flags = PTE_PRESENT | PTE_WRITE;

    for (uint32_t i = 0; i < page_count; i++) {
        paging_map_page(virt_addr + i * PAGE_SIZE,
                        phys_addr + i * PAGE_SIZE,
                        flags);
    }

    // Create new block
    heap_block_t* block = (heap_block_t*)virt_addr;
    block->size = page_count * PAGE_SIZE;
    block->used = 1;
    block->prev = 0;
    block->next = 0;

    // Update heap end
    g_heap_end += page_count * PAGE_SIZE;

    // Update statistics
    g_total_size += block->size;
    g_total_blocks++;
    g_used_blocks++;
    g_used_size += (block->size - sizeof(heap_block_t));

    printk("Heap expanded: +%d bytes at 0x%x (total: %d bytes)\n",
           block->size, virt_addr, g_heap_end - KERNEL_HEAP_START);

    return block;
}

// Initialize kernel heap
void heap_init() {
    printk("Initializing kernel heap...\n");
    printk("  Heap start: 0x%x\n", KERNEL_HEAP_START);
    printk("  Heap end: 0x%x\n", KERNEL_HEAP_END);
    printk("  Heap size: %d MB\n", KERNEL_HEAP_SIZE / (1024 * 1024));
    printk("Heap initialized\n");
}

// Allocate memory from kernel heap
void* kmalloc(uint32_t size) {
    if (size == 0) {
        return 0;
    }

    // Align size and add header
    uint32_t total_size = align_size(size) + sizeof(heap_block_t);

    // Find a free block that fits (first-fit strategy)
    heap_block_t* block = g_free_list;

    while (block) {
        if (!block->used && block->size >= total_size) {
            // Check if we can split this block
            if (block->size >= total_size + sizeof(heap_block_t) + 4) {
                // Split the block
                heap_block_t* new_block = (heap_block_t*)((uint32_t)block + total_size);
                new_block->size = block->size - total_size;
                new_block->used = 0;

                // Update current block size
                block->size = total_size;

                // Add new block to free list
                add_to_free_list(new_block);
                g_total_blocks++;

                // Update statistics
                g_used_size += (block->size - sizeof(heap_block_t));
                g_used_blocks++;
            }

            // Remove from free list and mark as used
            remove_from_free_list(block);

            printk("kmalloc(%d) = 0x%x (block size: %d)\n", size, block_to_ptr(block), block->size);

            return block_to_ptr(block);
        }
        block = block->next;
    }

    // No suitable block found, expand heap
    printk("No free block found, expanding heap...\n");
    block = expand_heap(total_size);
    if (!block) {
        printk("ERROR: Failed to expand heap for allocation of %d bytes\n", size);
        return 0;
    }

    printk("kmalloc(%d) = 0x%x (block size: %d, expanded heap)\n", size, block_to_ptr(block), block->size);

    return block_to_ptr(block);
}

// Free memory allocated with kmalloc
void kfree(void* ptr) {
    if (!ptr) {
        printk("WARNING: kfree called with null pointer\n");
        return;
    }

    // Get block from pointer
    heap_block_t* block = ptr_to_block(ptr);

    // Validate block
    if (!is_heap_addr((uint32_t)block)) {
        printk("ERROR: Invalid pointer passed to kfree: 0x%x\n", ptr);
        return;
    }

    if (!block->used) {
        printk("WARNING: Double free detected at 0x%x\n", ptr);
        return;
    }

    printk("kfree(0x%x) - block size: %d\n", ptr, block->size);

    // Add to free list
    add_to_free_list(block);

    // Coalesce with adjacent blocks
    coalesce_blocks(block);
}

// Get heap statistics
uint32_t heap_get_total_size() {
    return g_total_size;
}

uint32_t heap_get_used_size() {
    return g_used_size;
}

uint32_t heap_get_free_size() {
    return g_total_size - g_used_size;
}

uint32_t heap_get_total_blocks() {
    return g_total_blocks;
}

uint32_t heap_get_used_blocks() {
    return g_used_blocks;
}

uint32_t heap_get_free_blocks() {
    return g_total_blocks - g_used_blocks;
}

// Print heap statistics and dump heap blocks
void heap_print_stats() {
    printk("=== Kernel Heap Statistics ===\n");
    printk("Total size: %d bytes (%d KB)\n", g_total_size, g_total_size / 1024);
    printk("Used size: %d bytes (%d KB)\n", g_used_size, g_used_size / 1024);
    printk("Free size: %d bytes (%d KB)\n", g_total_size - g_used_size, (g_total_size - g_used_size) / 1024);
    printk("Total blocks: %d\n", g_total_blocks);
    printk("Used blocks: %d\n", g_used_blocks);
    printk("Free blocks: %d\n", g_total_blocks - g_used_blocks);
    printk("Heap range: 0x%x - 0x%x\n", KERNEL_HEAP_START, g_heap_end);

    // Dump all blocks
    printk("\n=== Heap Block Dump ===\n");
    uint32_t addr = KERNEL_HEAP_START;
    int count = 0;

    while (addr < g_heap_end && is_heap_addr(addr)) {
        heap_block_t* block = (heap_block_t*)addr;
        if (!is_heap_addr(addr + block->size)) {
            break;
        }

        printk("  Block %d: 0x%x - 0x%x, size: %d, %s\n",
               count++, addr, addr + block->size, block->size,
               block->used ? "USED" : "FREE");

        addr += block->size;
    }

    // Dump free list
    printk("\n=== Free List ===\n");
    heap_block_t* block = g_free_list;
    int free_count = 0;
    while (block) {
        printk("  Free block %d: 0x%x, size: %d\n", free_count++, (uint32_t)block, block->size);
        block = block->next;
    }

    printk("===========================\n");
}
