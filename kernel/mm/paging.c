#include "mm/paging.h"
#include "mm/mm.h"
#include "mm/heap.h"
#include "printk.h"

// Page directory and page tables
pde_t g_page_dir[1024] PAGE_ALIGN;

// Static page tables for early identity mapping (before PMM is ready)
// These cover the first 8MB of physical memory:
// - g_identity_map_page_table_0: maps 0x00000000 - 0x003FFFFF (4MB)
// - g_identity_map_page_table_1: maps 0x00400000 - 0x007FFFFF (4MB)
pte_t g_identity_map_page_table_0[1024] PAGE_ALIGN;
pte_t g_identity_map_page_table_1[1024] PAGE_ALIGN;

// Map a virtual address to a physical address
void paging_map_page(uint32_t virt_addr, uint32_t phys_addr, uint32_t flags) {
    // Extract page directory and table indices
    uint32_t pd_index = virt_addr >> 22;
    uint32_t pt_index = (virt_addr >> 12) & 0x3FF;

    // Get or create page table
    pde_t* pde = &g_page_dir[pd_index];

    if (!pde_is_present(pde)) {
        // Page table not present, need to allocate one
        uint32_t pt_addr;
        pte_t* pt;

        // For the first 2 page tables (pd_index = 0, 1), use the statically allocated page tables
        // This avoids issues when PMM cannot allocate low memory pages during early init
        if (pd_index == 0) {
            pt_addr = (uint32_t)g_identity_map_page_table_0;
            pt = g_identity_map_page_table_0;
        } else if (pd_index == 1) {
            pt_addr = (uint32_t)g_identity_map_page_table_1;
            pt = g_identity_map_page_table_1;
        } else {
            pt_addr = pmm_alloc_page();
            if (pt_addr == 0) {
                printk("ERROR: Failed to allocate page table for virtual address 0x%x\n", virt_addr);
                return;
            }
            pt = (pte_t*)pt_addr;

            // Clear the new page table
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

// Page fault ISR (called from assembly)
void page_fault_isr(registers_ptr_t* regs, uint32_t error_code, uint32_t fault_addr) {

    // Print page fault information
    printk("\n");
    printk("============================\n");
    printk("!!! PAGE FAULT !!!\n");
    printk("============================\n");
    printk("Fault address: 0x%x\n", fault_addr);
    printk("Error code: 0x%x (", error_code);

    // Parse error code
    int first = 1;

    if (error_code & PF_PRESENT) {
        printk("%sPage not present", first ? "" : ", ");
        first = 0;
    }
    if (error_code & PF_WRITE) {
        printk("%sWrite access", first ? "" : ", ");
        first = 0;
    }
    if (error_code & PF_USER) {
        printk("%sUser mode", first ? "" : ", ");
        first = 0;
    } else {
        printk("%sKernel mode", first ? "" : ", ");
        first = 0;
    }
    if (error_code & PF_RESERVED) {
        printk("%sReserved bit set", first ? "" : ", ");
        first = 0;
    }
    if (error_code & PF_FETCH) {
        printk("%sInstruction fetch", first ? "" : ", ");
        first = 0;
    }
    printk(")\n");

    // Print register information
    printk("\nRegister state:\n");
    printk("  EIP: 0x%x\n", regs->eip);
    printk("  ESP: 0x%x\n", regs->esp);
    printk("  EAX: 0x%x, EBX: 0x%x, ECX: 0x%x, EDX: 0x%x\n",
           regs->eax, regs->ebx, regs->ecx, regs->edx);
    printk("  ESI: 0x%x, EDI: 0x%x, EBP: 0x%x\n",
           regs->esi, regs->edi, regs->ebp);
    printk("  CS: 0x%x, DS: 0x%x, SS: 0x%x\n",
           regs->cs, regs->ds, regs->ss);
    printk("  EFLAGS: 0x%x\n", regs->eflags);

    // Check if fault address is in known regions
    printk("\nMemory region analysis:\n");
    if (fault_addr < 8 * 1024 * 1024) {
        printk("  Address is in identity-mapped region (0x0 - 0x800000)\n");
    } else if (fault_addr >= KERNEL_HEAP_START && fault_addr < KERNEL_HEAP_END) {
        printk("  Address is in kernel heap (0x%x - 0x%x)\n", KERNEL_HEAP_START, KERNEL_HEAP_END);
    } else if (fault_addr >= 0xFFC00000) {
        printk("  Address is in recursive page directory mapping\n");
    } else {
        printk("  Address is in unmapped region\n");
    }

    // Try to determine cause
    printk("\nPossible cause: ");
    if (!(error_code & PF_PRESENT)) {
        if (error_code & PF_USER) {
            printk("User process accessed unmapped memory\n");
        } else {
            printk("Kernel accessed unmapped memory (NULL pointer dereference?)\n");
        }
    } else if (error_code & PF_WRITE) {
        printk("Write access to read-only page\n");
    } else if (error_code & PF_USER) {
        printk("User process accessed kernel memory\n");
    } else if (error_code & PF_RESERVED) {
        printk("Reserved bit in page table entry was set\n");
    } else {
        printk("Unknown page fault\n");
    }

    printk("\nKernel panic: Unhandled page fault. System halted.\n");
    printk("============================\n\n");

    // Halt the system
    while (1) {
        __asm__ volatile ("hlt");
    }
}

// Get current page protection flags for a virtual address
// Returns: flags if page is mapped, 0 if not mapped
uint32_t paging_get_page_flags(uint32_t virt_addr) {
    uint32_t pd_index = virt_addr >> 22;
    uint32_t pt_index = (virt_addr >> 12) & 0x3FF;

    pde_t* pde = &g_page_dir[pd_index];

    if (!pde_is_present(pde)) {
        return 0;  // Page directory entry not present
    }

    pte_t* page_table = (pte_t*)(pde_get_frame(pde));
    pte_t* pte = &page_table[pt_index];

    if (!pte_is_present(pte)) {
        return 0;  // Page not mapped
    }

    // Build flags from PTE
    uint32_t flags = 0;
    if (pte->present) flags |= PTE_PRESENT;
    if (pte->write) flags |= PTE_WRITE;
    if (pte->user) flags |= PTE_USER;
    if (pte->pwt) flags |= PTE_PWT;
    if (pte->pcd) flags |= PTE_PCD;
    if (pte->global) flags |= PTE_GLOBAL;

    return flags;
}

// Change page protection flags
void paging_set_page_flags(uint32_t virt_addr, uint32_t flags) {
    uint32_t pd_index = virt_addr >> 22;
    uint32_t pt_index = (virt_addr >> 12) & 0x3FF;

    pde_t* pde = &g_page_dir[pd_index];

    if (!pde_is_present(pde)) {
        printk("ERROR: Cannot set flags for unmapped page at 0x%x\n", virt_addr);
        return;
    }

    pte_t* page_table = (pte_t*)(pde_get_frame(pde));
    pte_t* pte = &page_table[pt_index];

    if (!pte_is_present(pte)) {
        printk("ERROR: Cannot set flags for unmapped page at 0x%x\n", virt_addr);
        return;
    }

    // Update flags
    pte->present = (flags & PTE_PRESENT) ? 1 : 0;
    pte->write = (flags & PTE_WRITE) ? 1 : 0;
    pte->user = (flags & PTE_USER) ? 1 : 0;
    pte->pwt = (flags & PTE_PWT) ? 1 : 0;
    pte->pcd = (flags & PTE_PCD) ? 1 : 0;
    pte->global = (flags & PTE_GLOBAL) ? 1 : 0;

    // Invalidate TLB entry for this page
    paging_invalidate_page(virt_addr);
}

// Unmap a virtual address (remove page mapping)
void paging_unmap_page(uint32_t virt_addr) {
    uint32_t pd_index = virt_addr >> 22;
    uint32_t pt_index = (virt_addr >> 12) & 0x3FF;

    pde_t* pde = &g_page_dir[pd_index];

    if (!pde_is_present(pde)) {
        return;  // Already unmapped
    }

    pte_t* page_table = (pte_t*)(pde_get_frame(pde));
    pte_t* pte = &page_table[pt_index];

    if (!pte_is_present(pte)) {
        return;  // Already unmapped
    }

    // Mark page as not present
    pte->present = 0;

    // Invalidate TLB entry
    paging_invalidate_page(virt_addr);
}

// Map multiple pages (contiguous virtual to physical)
void paging_map_pages(uint32_t virt_addr, uint32_t phys_addr, uint32_t page_count, uint32_t flags) {
    for (uint32_t i = 0; i < page_count; i++) {
        paging_map_page(virt_addr + i * PAGE_SIZE,
                        phys_addr + i * PAGE_SIZE,
                        flags);
    }
}

// Unmap multiple pages
void paging_unmap_pages(uint32_t virt_addr, uint32_t page_count) {
    for (uint32_t i = 0; i < page_count; i++) {
        paging_unmap_page(virt_addr + i * PAGE_SIZE);
    }
}

// Invalidate a specific page in TLB
void paging_invalidate_page(uint32_t virt_addr) {
    __asm__ volatile (
        "invlpg (%0)"
        :
        : "r"(virt_addr)
        : "memory"
    );
}

// Flush TLB (Translation Lookaside Buffer)
void paging_flush_tlb() {
    // Reload CR3 to flush entire TLB
    uint32_t cr3;
    __asm__ volatile (
        "movl %%cr3, %0"
        : "=r"(cr3)
    );
    __asm__ volatile (
        "movl %0, %%cr3"
        :
        : "r"(cr3)
    );
}

// Get page directory address
uint32_t paging_get_page_dir() {
    uint32_t cr3;
    __asm__ volatile (
        "movl %%cr3, %0"
        : "=r"(cr3)
    );
    return cr3;
}

// Print page table statistics
void paging_print_stats() {
    printk("=== Paging Statistics ===\n");
    printk("Page directory at: 0x%x\n", (uint32_t)g_page_dir);

    uint32_t pd_entries = 0;
    uint32_t pt_entries = 0;

    for (int i = 0; i < 1024; i++) {
        if (g_page_dir[i].present) {
            pd_entries++;

            pte_t* page_table = (pte_t*)(pde_get_frame(&g_page_dir[i]));
            for (int j = 0; j < 1024; j++) {
                if (page_table[j].present) {
                    pt_entries++;
                }
            }
        }
    }

    printk("Page directory entries in use: %d / 1024\n", pd_entries);
    printk("Page table entries in use: %d / 1048576\n", pt_entries);
    printk("Memory mapped: %d MB\n", (pt_entries * PAGE_SIZE) / (1024 * 1024));
    printk("==========================\n");
}

// Make a page read-only (remove write permission)
void paging_make_read_only(uint32_t virt_addr) {
    uint32_t flags = paging_get_page_flags(virt_addr);
    if (flags == 0) {
        printk("WARNING: Cannot make unmapped page read-only at 0x%x\n", virt_addr);
        return;
    }

    flags &= ~PTE_WRITE;  // Clear write bit
    paging_set_page_flags(virt_addr, flags);

    printk("Made page at 0x%x read-only\n", virt_addr);
}

// Make a page writable (add write permission)
void paging_make_writable(uint32_t virt_addr) {
    uint32_t flags = paging_get_page_flags(virt_addr);
    if (flags == 0) {
        printk("WARNING: Cannot make unmapped page writable at 0x%x\n", virt_addr);
        return;
    }

    flags |= PTE_WRITE;  // Set write bit
    paging_set_page_flags(virt_addr, flags);

    printk("Made page at 0x%x writable\n", virt_addr);
}

// Make a page accessible to user mode (add user permission)
void paging_make_user_accessible(uint32_t virt_addr) {
    uint32_t flags = paging_get_page_flags(virt_addr);
    if (flags == 0) {
        printk("WARNING: Cannot make unmapped page user-accessible at 0x%x\n", virt_addr);
        return;
    }

    flags |= PTE_USER;  // Set user bit
    paging_set_page_flags(virt_addr, flags);

    printk("Made page at 0x%x user-accessible\n", virt_addr);
}

// Make a page kernel-only (remove user permission)
void paging_make_kernel_only(uint32_t virt_addr) {
    uint32_t flags = paging_get_page_flags(virt_addr);
    if (flags == 0) {
        printk("WARNING: Cannot make unmapped page kernel-only at 0x%x\n", virt_addr);
        return;
    }

    flags &= ~PTE_USER;  // Clear user bit
    paging_set_page_flags(virt_addr, flags);

    printk("Made page at 0x%x kernel-only\n", virt_addr);
}

// Check if an address is accessible
// Returns: 1 if accessible, 0 if not
int paging_is_accessible(uint32_t virt_addr, int is_write, int is_user_mode) {
    uint32_t pd_index = virt_addr >> 22;
    uint32_t pt_index = (virt_addr >> 12) & 0x3FF;

    pde_t* pde = &g_page_dir[pd_index];

    // Check if page directory entry is present
    if (!pde_is_present(pde)) {
        return 0;
    }

    // Check user/kernel permission at page directory level
    if (is_user_mode && !pde->user) {
        return 0;
    }

    pte_t* page_table = (pte_t*)(pde_get_frame(pde));
    pte_t* pte = &page_table[pt_index];

    // Check if page table entry is present
    if (!pte_is_present(pte)) {
        return 0;
    }

    // Check write permission
    if (is_write && !pte->write) {
        return 0;
    }

    // Check user/kernel permission at page table level
    if (is_user_mode && !pte->user) {
        return 0;
    }

    return 1;
}

// Page fault handler (wrapper called from C code)
void page_fault_handler(uint32_t error_code, uint32_t fault_addr, registers_ptr_t* regs) {
    page_fault_isr(regs, error_code, fault_addr);
}
