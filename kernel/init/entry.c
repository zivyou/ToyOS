#include "types.h"
#include "terminal.h"
#include "printk.h"
#include "common.h"
#include "mm/mm.h"
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
    while (1) {
        printk("kernel main loop begin....\n");
        pmm_print_stats();
        uint32_t pages = pmm_alloc_pages(4);
        if (pages != 0) {
            printk("allocated 4 pages at 0x%x\n", pages);
        }
        pmm_print_stats();
        printk("test: %d", 1/0);
        /*
        https://stackoverflow.com/questions/54724812/os-dev-general-protection-fault-problem-after-setting-up-idt
        */
        hlt();
        printk("kernel main loop end....\n");
    }
    return 0;
}