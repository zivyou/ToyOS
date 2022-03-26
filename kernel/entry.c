#include <types.h>
#include <terminal.h>
#include <printk.h>
#include <common.h>


extern void gdt_init();
extern void idt_init();
extern void intr_init();

int kern_entry(int*, int*)__attribute__((cdecl));

int kern_entry(int* num1, int* num2){
    terminal_init();
    gdt_init();
    idt_init();
    (*num1)++; (*num2)++;
    printk("hello world!\n");

    int int0 = 0;
    printk("%s, %d\n", "welcome!", *num1);
    hlt();

    return 0;
}