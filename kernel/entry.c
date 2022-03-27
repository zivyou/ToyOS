#include <types.h>
#include <terminal.h>
#include <printk.h>
#include <common.h>


extern void gdt_init();
extern void idt_init();
extern void intr_init();

_Noreturn int kern_entry(){
    terminal_init();
    printk("hello world!\n");
    int int0 = 0;
    printk("welcome!\n");
    gdt_init();
    idt_init();

    int0++;
    while (1) hlt();
    __asm__ volatile ("sti");
    return 0;
}