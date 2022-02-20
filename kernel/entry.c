#include <types.h>
#include <terminal.h>
#include <printk.h>
#include <common.h>


extern void gdt_init();
extern void idt_init();
extern void intr_init();

int kern_entry(){
    gdt_init();
    idt_init();
    terminal_init();

    printk("hello world!\n");

    int int0 = 0;
    printk("%s\n", "welcome!");
    int b = 10/int0;
    hlt();
    return 0;
}