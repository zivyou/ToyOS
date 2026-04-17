#ifndef PAGING_H
#define PAGING_H

#include "types.h"
#include "mm/mm.h"

// Page directory entry flags
#define PDE_PRESENT    (1 << 0)
#define PDE_WRITE      (1 << 1)
#define PDE_USER       (1 << 2)
#define PDE_PWT        (1 << 3)
#define PDE_PCD        (1 << 4)
#define PDE_ACCESSED   (1 << 5)
#define PDE_DIRTY      (1 << 6)
#define PDE_PAGE_SIZE  (1 << 7)  // 4MB pages if set
#define PDE_GLOBAL     (1 << 8)

// Page table entry flags
#define PTE_PRESENT    (1 << 0)
#define PTE_WRITE      (1 << 1)
#define PTE_USER       (1 << 2)
#define PTE_PWT        (1 << 3)
#define PTE_PCD        (1 << 4)
#define PTE_ACCESSED   (1 << 5)
#define PTE_DIRTY      (1 << 6)
#define PTE_PAT        (1 << 7)
#define PTE_GLOBAL     (1 << 8)

// Alignment for page directories/tables
#define PAGE_ALIGN __attribute__((aligned(PAGE_SIZE)))

// Page directory entry structure
typedef struct {
    uint32_t present    : 1;
    uint32_t write      : 1;
    uint32_t user       : 1;
    uint32_t pwt        : 1;
    uint32_t pcd        : 1;
    uint32_t accessed   : 1;
    uint32_t dirty      : 1;
    uint32_t page_size  : 1;
    uint32_t global     : 1;
    uint32_t available  : 3;
    uint32_t frame      : 20;
} __attribute__((packed)) pde_t;

// Page table entry structure
typedef struct {
    uint32_t present    : 1;
    uint32_t write      : 1;
    uint32_t user       : 1;
    uint32_t pwt        : 1;
    uint32_t pcd        : 1;
    uint32_t accessed   : 1;
    uint32_t dirty      : 1;
    uint32_t pat        : 1;
    uint32_t global     : 1;
    uint32_t available  : 3;
    uint32_t frame      : 20;
} __attribute__((packed)) pte_t;

// Get frame address from PTE/PDE
static inline uint32_t pte_get_frame(pte_t* pte) {
    return pte->frame << 12;
}

static inline uint32_t pde_get_frame(pde_t* pde) {
    return pde->frame << 12;
}

// Set frame address in PTE/PDE
static inline void pte_set_frame(pte_t* pte, uint32_t addr) {
    pte->frame = addr >> 12;
}

static inline void pde_set_frame(pde_t* pde, uint32_t addr) {
    pde->frame = addr >> 12;
}

// Check if PTE/PDE is present
static inline int pte_is_present(pte_t* pte) {
    return pte->present;
}

static inline int pde_is_present(pde_t* pde) {
    return pde->present;
}

// Enable/disable present bit
static inline void pte_set_present(pte_t* pte, int val) {
    pte->present = val;
}

static inline void pde_set_present(pde_t* pde, int val) {
    pde->present = val;
}

// Page directory and table
extern pde_t g_page_dir[1024] PAGE_ALIGN;

// Static page tables for early identity mapping
extern pte_t g_identity_map_page_table_0[1024] PAGE_ALIGN;  // Maps 0x0 - 0x3FFFFF
extern pte_t g_identity_map_page_table_1[1024] PAGE_ALIGN;  // Maps 0x400000 - 0x7FFFFF

// Paging functions
void paging_init();
void paging_load_cr3(uint32_t page_dir_phys);
void paging_enable();

// Map a virtual address to a physical address
void paging_map_page(uint32_t virt_addr, uint32_t phys_addr, uint32_t flags);

// Unmap a virtual address (remove page mapping)
void paging_unmap_page(uint32_t virt_addr);

// Map multiple pages (contiguous virtual to physical)
void paging_map_pages(uint32_t virt_addr, uint32_t phys_addr, uint32_t page_count, uint32_t flags);

// Unmap multiple pages
void paging_unmap_pages(uint32_t virt_addr, uint32_t page_count);

// Get physical address for a virtual address
uint32_t paging_get_phys_addr(uint32_t virt_addr);

// Change page protection flags
void paging_set_page_flags(uint32_t virt_addr, uint32_t flags);

// Flush TLB (Translation Lookaside Buffer)
void paging_flush_tlb();

// Get page directory address
uint32_t paging_get_page_dir();

// Invalidate a specific page in TLB
void paging_invalidate_page(uint32_t virt_addr);

// Print page table statistics
void paging_print_stats();

#endif // PAGING_H
