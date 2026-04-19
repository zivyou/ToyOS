#include "types.h"
#include "terminal.h"
#include "printk.h"
#include "common.h"
#include "mm/mm.h"
#include "mm/heap.h"
#include "mm/paging.h"
#include "multiboot.h"



extern void gdt_init();
extern void idt_init();
extern void intr_init();

_Noreturn int kern_entry(uint32_t magic, uint32_t info_ptr){
    terminal_init();
    multiboot_init(magic, (multiboot_info_t*)info_ptr);
    printk("hello world!\n");
    printk("welcome!\n");
    gdt_init();
    idt_init();
    mm_init();
    __asm__ volatile ("sti");

    // Test kmalloc/kfree
    printk("\n=== Testing kmalloc/kfree ===\n");

    // Test 1: Simple allocation
    char* ptr1 = kmalloc(100);
    if (ptr1) {
        printk("Test 1: Allocated 100 bytes at 0x%x\n", ptr1);
        for (int i = 0; i < 100; i++) {
            ptr1[i] = 'A' + (i % 26);
        }
        printk("  Data verification: ptr1[0]='%c', ptr1[25]='%c', ptr1[99]='%c'\n",
               ptr1[0], ptr1[25], ptr1[99]);
    }

    // Test 2: Multiple allocations
    void* ptr2 = kmalloc(256);
    void* ptr3 = kmalloc(512);
    void* ptr4 = kmalloc(1024);
    printk("Test 2: Allocated 256 bytes at 0x%x\n", ptr2);
    printk("Test 3: Allocated 512 bytes at 0x%x\n", ptr3);
    printk("Test 4: Allocated 1024 bytes at 0x%x\n", ptr4);

    heap_print_stats();

    // Test 5: Free and reallocate
    printk("\nTest 5: Freeing ptr2 (256 bytes)...\n");
    kfree(ptr2);
    heap_print_stats();

    printk("\nTest 6: Freeing ptr1 (100 bytes)...\n");
    kfree(ptr1);
    heap_print_stats();

    // Test 7: Allocate after free (should reuse freed space)
    void* ptr5 = kmalloc(200);
    printk("Test 7: Allocated 200 bytes at 0x%x (after free)\n", ptr5);
    heap_print_stats();

    // Test 8: Large allocation (will trigger heap expansion)
    void* ptr6 = kmalloc(8192);
    printk("Test 8: Allocated 8192 bytes at 0x%x (large allocation)\n", ptr6);
    heap_print_stats();

    // Clean up
    kfree(ptr3);
    kfree(ptr4);
    kfree(ptr5);
    kfree(ptr6);

    printk("\n=== Final heap statistics ===\n");
    heap_print_stats();

    printk("\n=== kmalloc/kfree tests completed ===\n\n");

    // Memory protection tests
    printk("=== Testing Memory Protection ===\n");

    // Test 1: Get page flags for known addresses
    printk("\nTest 1: Getting page flags...\n");
    uint32_t flags = paging_get_page_flags(0x1000);
    printk("  Page flags for 0x1000: 0x%x\n", flags);

    flags = paging_get_page_flags((uint32_t)ptr6);
    printk("  Page flags for heap ptr6 (0x%x): 0x%x\n", ptr6, flags);

    flags = paging_get_page_flags((uint32_t)kern_entry);
    printk("  Page flags for kern_entry function (0x%x): 0x%x\n",
           (uint32_t)kern_entry, flags);

    // Test 2: Check accessibility
    printk("\nTest 2: Checking accessibility...\n");
    int accessible = paging_is_accessible(0x1000, 0, 0);  // Read, kernel
    printk("  0x1000 readable by kernel: %s\n", accessible ? "YES" : "NO");

    accessible = paging_is_accessible(0x1000, 1, 0);  // Write, kernel
    printk("  0x1000 writable by kernel: %s\n", accessible ? "YES" : "NO");

    // Test 3: Make a page read-only
    printk("\nTest 3: Making page read-only...\n");
    uint32_t test_addr = (uint32_t)ptr5;
    printk("  Original flags for 0x%x: 0x%x\n", test_addr,
           paging_get_page_flags(test_addr));
    paging_make_read_only(test_addr);
    printk("  After make_read_only: 0x%x\n",
           paging_get_page_flags(test_addr));

    // Test 4: Make it writable again
    printk("\nTest 4: Making page writable again...\n");
    paging_make_writable(test_addr);
    printk("  After make_writable: 0x%x\n",
           paging_get_page_flags(test_addr));

    // Test 5: User/kernel accessibility
    printk("\nTest 5: User/kernel accessibility...\n");
    printk("  Original flags: 0x%x\n", paging_get_page_flags(test_addr));
    paging_make_user_accessible(test_addr);
    printk("  After make_user_accessible: 0x%x\n",
           paging_get_page_flags(test_addr));

    paging_make_kernel_only(test_addr);
    printk("  After make_kernel_only: 0x%x\n",
           paging_get_page_flags(test_addr));

    // Test 6: Print paging statistics
    printk("\nTest 6: Paging statistics...\n");
    paging_print_stats();

    // Test 7: Unmap test (careful - don't unmap kernel code)
    printk("\nTest 7: Testing unmapping...\n");
    // Allocate a page for testing
    uint32_t test_phys = pmm_alloc_page();
    if (test_phys) {
        uint32_t test_virt = 0xC0000000;  // Use high memory for test
        paging_map_page(test_virt, test_phys, PTE_PRESENT | PTE_WRITE);
        printk("  Mapped test page: 0x%x -> 0x%x\n", test_virt, test_phys);

        // Write to it
        volatile uint32_t* test_ptr = (uint32_t*)test_virt;
        *test_ptr = 0xDEADBEEF;
        printk("  Wrote 0xDEADBEEF, read back: 0x%x\n", *test_ptr);

        // Unmap it
        paging_unmap_page(test_virt);
        printk("  Unmapped test page\n");

        // Check if it's still accessible
        flags = paging_get_page_flags(test_virt);
        printk("  Page flags after unmap: 0x%x\n", flags);

        // Free the physical page
        pmm_free_page(test_phys);
    }

    printk("\n=== Memory protection tests completed ===\n\n");
    while (1) {
        // printk("kernel main loop begin....\n");
        /*
        https://stackoverflow.com/questions/54724812/os-dev-general-protection-fault-problem-after-setting-up-idt
        */
        hlt();
        // printk("kernel main loop end....\n");
    }
    return 0;
}