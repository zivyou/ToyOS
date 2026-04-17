#include "types.h"
#include "terminal.h"
#include "printk.h"
#include "common.h"
#include "mm/mm.h"



extern void gdt_init();
extern void idt_init();
extern void intr_init();

_Noreturn int kern_entry(){
    terminal_init();
    printk("hello world!\n");
    printk("welcome!\n");
    gdt_init();
    idt_init();
    mm_init();
    __asm__ volatile ("sti");
    while (1) {
        printk("kernel main loop begin....\n");
        printk("test: %d", 1/0);
        /*
        https://stackoverflow.com/questions/54724812/os-dev-general-protection-fault-problem-after-setting-up-idt
        */
        hlt();
        printk("kernel main loop end....\n");
    }
    return 0;
}