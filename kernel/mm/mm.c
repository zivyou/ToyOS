//
// Created by ziv on 2022/3/31.
//
#include "mm/mm.h"
#include "mm/paging.h"
#include "printk.h"
#include "multiboot.h"

// External reference to multiboot info
extern multiboot_info_t* g_multiboot_info;

// Physical memory bitmap
// Each bit represents one 4KB page
// 32 bits * 1024 words = 32768 bits = 32768 pages = 128MB (can be expanded)
uint32_t g_pmm_bitmap[BITMAP_SIZE] = {0};

// Track total pages
uint32_t g_total_pages = 0;
uint32_t g_free_pages = 0;

// Helper functions for bitmap manipulation
static inline void pmm_set_bit(uint32_t bit) {
    g_pmm_bitmap[bit / 32] |= (1 << (bit % 32));
}

static inline void pmm_clear_bit(uint32_t bit) {
    g_pmm_bitmap[bit / 32] &= ~(1 << (bit % 32));
}

static inline int pmm_test_bit(uint32_t bit) {
    return g_pmm_bitmap[bit / 32] & (1 << (bit % 32));
}

// Mark a memory region as used or free
void pmm_mark_region(uint32_t start, uint32_t end, int used) {
    // Align to page boundaries
    uint32_t start_page = start / PAGE_SIZE;
    uint32_t end_page = end / PAGE_SIZE;

    for (uint32_t i = start_page; i < end_page; i++) {
        if (used) {
            pmm_set_bit(i);
            if (g_free_pages > 0) g_free_pages--;
        } else {
            pmm_clear_bit(i);
            g_free_pages++;
        }
    }
}

// Allocate a physical page
// Returns: physical address of allocated page, 0 on failure
uint32_t pmm_alloc_page() {
    for (uint32_t i = 0; i < g_total_pages; i++) {
        if (!pmm_test_bit(i)) {
            pmm_set_bit(i);
            g_free_pages--;
            return i * PAGE_SIZE;
        }
    }
    printk("ERROR: Out of memory!\n");
    return 0;  // Out of memory
}

// Free a physical page
void pmm_free_page(uint32_t page_addr) {
    uint32_t page = page_addr / PAGE_SIZE;
    if (page < g_total_pages) {
        if (!pmm_test_bit(page)) {
            printk("WARNING: Double free of page 0x%x\n", page_addr);
            return;
        }
        pmm_clear_bit(page);
        g_free_pages++;
    }
}

// Allocate multiple contiguous pages
// Returns: physical address of first allocated page, 0 on failure
uint32_t pmm_alloc_pages(uint32_t page_count) {
    if (page_count == 0) {
        printk("WARNING: pmm_alloc_pages called with count=0\n");
        return 0;
    }

    if (page_count > g_free_pages) {
        printk("ERROR: Cannot allocate %d pages, only %d free\n", page_count, g_free_pages);
        return 0;
    }

    // Find contiguous free pages
    uint32_t contiguous = 0;
    for (uint32_t i = 0; i < g_total_pages; i++) {
        if (!pmm_test_bit(i)) {
            contiguous++;
            if (contiguous == page_count) {
                // Found enough contiguous pages, allocate them
                uint32_t start_page = i - page_count + 1;
                uint32_t start_addr = start_page * PAGE_SIZE;

                for (uint32_t j = start_page; j <= i; j++) {
                    pmm_set_bit(j);
                    g_free_pages--;
                }

                printk("Allocated %d contiguous pages at 0x%x\n", page_count, start_addr);
                return start_addr;
            }
        } else {
            contiguous = 0;
        }
    }

    printk("ERROR: Could not find %d contiguous pages\n", page_count);
    return 0;
}

// Free multiple contiguous pages
void pmm_free_pages(uint32_t page_addr, uint32_t page_count) {
    uint32_t start_page = page_addr / PAGE_SIZE;

    if (start_page >= g_total_pages) {
        printk("ERROR: Invalid page address 0x%x\n", page_addr);
        return;
    }

    if (start_page + page_count > g_total_pages) {
        printk("ERROR: Freeing %d pages starting at 0x%x exceeds memory\n", page_count, page_addr);
        return;
    }

    for (uint32_t i = start_page; i < start_page + page_count; i++) {
        if (!pmm_test_bit(i)) {
            printk("WARNING: Page 0x%x not allocated, skipping\n", i * PAGE_SIZE);
        } else {
            pmm_clear_bit(i);
            g_free_pages++;
        }
    }

    printk("Freed %d contiguous pages at 0x%x\n", page_count, page_addr);
}

// Get number of free pages
uint32_t pmm_get_free_pages() {
    return g_free_pages;
}

// Get total number of pages
uint32_t pmm_get_total_pages() {
    return g_total_pages;
}

// Print memory statistics
void pmm_print_stats() {
    printk("=== Memory Statistics ===\n");
    printk("Total pages: %d\n", g_total_pages);
    printk("Free pages: %d (%d MB)\n", g_free_pages, (g_free_pages * PAGE_SIZE) / (1024 * 1024));
    printk("Used pages: %d (%d MB)\n", g_total_pages - g_free_pages,
           ((g_total_pages - g_free_pages) * PAGE_SIZE) / (1024 * 1024));
    printk("========================\n");
}

// Initialize physical memory manager from multiboot memory map
void pmm_init() {
    printk("Initializing physical memory manager...\n");

    if (!g_multiboot_info || !(g_multiboot_info->flags & MULTIBOOT_FLAG_MMAP)) {
        printk("ERROR: No memory map available!\n");
        return;
    }

    // First pass: find the highest address to calculate total pages needed
    uint32_t max_addr = 0;
    multiboot_mmap_entry_t* mmap = (multiboot_mmap_entry_t*)g_multiboot_info->mmap_addr;

    while ((uint32_t)mmap < g_multiboot_info->mmap_addr + g_multiboot_info->mmap_length) {
        uint64_t end = ((uint64_t)mmap->addr_high << 32) + mmap->addr_low +
                       ((uint64_t)mmap->len_high << 32) + mmap->len_low;

        if (end > max_addr && end < 0xFFFFFFFFUL) {
            max_addr = (uint32_t)end;
        }

        mmap = (multiboot_mmap_entry_t*)((uint32_t)mmap + mmap->size + sizeof(mmap->size));
    }

    g_total_pages = max_addr / PAGE_SIZE;
    printk("Total physical memory: %d MB (%d pages)\n", max_addr / (1024 * 1024), g_total_pages);

    // Check if we have enough bitmap space
    if (g_total_pages > BITMAP_SIZE * 32) {
        printk("WARNING: Not enough bitmap space for all memory!\n");
        printk("  Bitmap can track %d pages, but system has %d pages\n", BITMAP_SIZE * 32, g_total_pages);
        g_total_pages = BITMAP_SIZE * 32;
    }

    // Second pass: initialize bitmap from memory map
    mmap = (multiboot_mmap_entry_t*)g_multiboot_info->mmap_addr;
    int region_count = 0;

    while ((uint32_t)mmap < g_multiboot_info->mmap_addr + g_multiboot_info->mmap_length) {
        uint32_t start = mmap->addr_low;
        uint32_t end = start + mmap->len_low;

        // Only handle 32-bit addresses for now
        if (mmap->addr_high == 0 && mmap->len_high == 0) {
            if (mmap->type == MULTIBOOT_MEMORY_AVAILABLE) {
                pmm_mark_region(start, end, 0);  // Mark as free
                region_count++;
            } else {
                pmm_mark_region(start, end, 1);  // Mark as used
            }
        }

        mmap = (multiboot_mmap_entry_t*)((uint32_t)mmap + mmap->size + sizeof(mmap->size));
    }

    printk("PMM initialized: %d/%d pages free\n", g_free_pages, g_total_pages);
}

void mm_init() {
    printk("mm initing.....\n");
    pmm_init();
    paging_init();
}