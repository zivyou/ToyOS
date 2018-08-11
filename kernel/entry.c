#include "types.h"
#include "terminal.h"
#include "printk.h"


int kern_entry(){
    terminal_init();
    printk("%%%d%s\n", 100, "test");
    return 0;
}