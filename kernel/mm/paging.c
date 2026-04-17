#include "mm/paging.h"
#include "mm/mm.h"
#include "printk.h"

// Page directory and page tables
pde_t g_page_dir[1024] PAGE_ALIGN;
pte_t g_page_tables[1024] PAGE_ALIGN;

// Map a virtual address to a physical address
void paging_map_page(uint32_t virt_addr, uint32_t phys_addr, uint32_t flags) {
    // Extract page directory and table indices
    uint32_t pd_index = virt_addr >> 22;
    uint32_t pt_index = (virt_addr >> 12) & 0x3FF;

    // Get or create page table
    pde_t* pde = &g_page_dir[pd_index];

    if (!pde_is_present(pde)) {
        // Page table not present, need to allocate one
        uint32_t pt_addr = pmm_alloc_page();
        if (pt_addr == 0) {
            printk("ERROR: Failed to allocate page table for virtual address 0x%x\n", virt_addr);
            return;
        }

        // Clear the new page table
        pte_t* pt = (pte_t*)pt_addr;
        for (int i = 0; i < 1024; i++) {
            pt[i].present = 0;
            pt[i].write = 0;
            pt[i].user = 0;
            pt[i].pwt = 0;
            pt[i].pcd = 0;
            pt[i].accessed = 0;
            pt[i].dirty = 0;
            pt[i].pat = 0;
            pt[i].global = 0;
            pt[i].available = 0;
            pt[i].frame = 0;
        }

        // Set up the page directory entry
        pde->present = 1;
        pde->write = 1;
        pde->user = 0;
        pde->pwt = 0;
        pde->pcd = 0;
        pde->accessed = 0;
        pde->dirty = 0;
        pde->page_size = 0;  // 4KB pages
        pde->global = 0;
        pde->available = 0;
        pde_set_frame(pde, pt_addr);
    }

    // Get page table address from page directory entry
    pte_t* page_table = (pte_t*)(pde_get_frame(pde));

    // Map the page
    pte_t* pte = &page_table[pt_index];
    pte->present = (flags & PTE_PRESENT) ? 1 : 0;
    pte->write = (flags & PTE_WRITE) ? 1 : 0;
    pte->user = (flags & PTE_USER) ? 1 : 0;
    pte->pwt = (flags & PTE_PWT) ? 1 : 0;
    pte->pcd = (flags & PTE_PCD) ? 1 : 0;
    pte->accessed = 0;
    pte->dirty = 0;
    pte->pat = 0;
    pte->global = (flags & PTE_GLOBAL) ? 1 : 0;
    pte->available = 0;
    pte_set_frame(pte, phys_addr);
}

// Get physical address for a virtual address
uint32_t paging_get_phys_addr(uint32_t virt_addr) {
    uint32_t pd_index = virt_addr >> 22;
    uint32_t pt_index = (virt_addr >> 12) & 0x3FF;
    uint32_t offset = virt_addr & 0xFFF;

    pde_t* pde = &g_page_dir[pd_index];

    if (!pde_is_present(pde)) {
        return 0;  // Page not mapped
    }

    pte_t* page_table = (pte_t*)(pde_get_frame(pde));
    pte_t* pte = &page_table[pt_index];

    if (!pte_is_present(pte)) {
        return 0;  // Page not mapped
    }

    return pte_get_frame(pte) + offset;
}

// Load page directory base address into CR3 register
void paging_load_cr3(uint32_t page_dir_phys) {
    printk("Loading CR3 with page directory at 0x%x\n", page_dir_phys);
    __asm__ volatile (
        "movl %0, %%cr3"
        :
        : "r"(page_dir_phys)
        : "%eax"
    );
}

// Enable paging by setting PG bit in CR0
void paging_enable() {
    printk("Enabling paging...\n");
    uint32_t cr0;

    // Read current CR0 value
    __asm__ volatile (
        "movl %%cr0, %0"
        : "=r"(cr0)
    );

    // Set PG (paging) bit (bit 31)
    cr0 |= 0x80000000;

    // Write back to CR0
    __asm__ volatile (
        "movl %0, %%cr0"
        :
        : "r"(cr0)
    );

    printk("Paging enabled!\n");
}

// Initialize paging
void paging_init() {
    printk("Initializing paging...\n");

    // Clear page directory
    for (int i = 0; i < 1024; i++) {
        g_page_dir[i].present = 0;
        g_page_dir[i].write = 0;
        g_page_dir[i].user = 0;
        g_page_dir[i].pwt = 0;
        g_page_dir[i].pcd = 0;
        g_page_dir[i].accessed = 0;
        g_page_dir[i].dirty = 0;
        g_page_dir[i].page_size = 0;
        g_page_dir[i].global = 0;
        g_page_dir[i].available = 0;
        g_page_dir[i].frame = 0;
    }

    // Identity map the first 8MB (kernel memory)
    // We need enough pages for the kernel code, data, and initial stack
    uint32_t identity_map_end = 8 * 1024 * 1024;  // 8MB
    uint32_t flags = PTE_PRESENT | PTE_WRITE;  // Kernel pages are read-write

    printk("Identity mapping first 8MB (0x0 - 0x%x)...\n", identity_map_end);

    for (uint32_t addr = 0; addr < identity_map_end; addr += PAGE_SIZE) {
        paging_map_page(addr, addr, flags);
    }

    // Map the page directory and page tables themselves
    // This allows us to access page tables at a known virtual address
    // Map page tables at virtual address 0xFFC00000 (last 4MB of address space)

    printk("Mapping page directory at 0xFFFFF000...\n");

    // Map the page directory at virtual address 0xFFFFF000
    // This is a recursive mapping trick that allows easy access to page tables
    // The page directory itself is mapped to the last entry in the page directory
    g_page_dir[1023].present = 1;
    g_page_dir[1023].write = 1;
    g_page_dir[1023].user = 0;
    g_page_dir[1023].pwt = 0;
    g_page_dir[1023].pcd = 0;
    g_page_dir[1023].accessed = 0;
    g_page_dir[1023].dirty = 0;
    g_page_dir[1023].page_size = 0;
    g_page_dir[1023].global = 0;
    g_page_dir[1023].available = 0;
    pde_set_frame(&g_page_dir[1023], (uint32_t)g_page_dir);

    printk("Page directory and tables initialized\n");
    printk("  Page directory at: 0x%x\n", (uint32_t)g_page_dir);

    // Print some statistics
    uint32_t mapped_pages = 0;
    for (int i = 0; i < 1024; i++) {
        if (g_page_dir[i].present) {
            mapped_pages++;
        }
    }
    printk("  Page directory entries in use: %d\n", mapped_pages);
}
