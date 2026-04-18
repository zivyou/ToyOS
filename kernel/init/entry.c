#include "types.h"
#include "terminal.h"
#include "printk.h"
#include "common.h"
#include "mm/mm.h"
#include "mm/heap.h"
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