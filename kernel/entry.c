#include <types.h>
#include <terminal.h>
#include <printk.h>


extern void gdt_init();
extern void idt_init();
extern void intr_init();
extern int_0;

int kern_entry(){
    gdt_init();
    idt_init();
    intr_init();
    terminal_init();

    printk("hello world!");
    
    /*
    int gdt=0;
    __asm__("sgdt %0":"=m"(gdt)::);
    printk("%x\n", gdt);
    */

    int int0 = 0;
    printk("%x\n", 1/int0);
    
    
    return 0;
}