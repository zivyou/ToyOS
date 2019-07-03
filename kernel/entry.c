#include <types.h>
#include <terminal.h>
#include <printk.h>


int kern_entry(){
    terminal_init();
    printk("%%%d%s\n", 100, "test");
    int *p = (int *)0x100000001;
    *p = 0;
    int a = 10;
    printk("%d %d", *p, a);
    return 0;
}