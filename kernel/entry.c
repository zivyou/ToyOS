#include <types.h>
#include <terminal.h>
#include <printk.h>


extern void gdt_init();
extern void idt_init();

int kern_entry(){
    gdt_init();
    idt_init();
    
    terminal_init();

    printk("hello world!");
    
    int gdt=0;
    __asm__("sgdt %0":"=m"(gdt)::);
    printk("%x\n", gdt);
    
    
    return 0;
}