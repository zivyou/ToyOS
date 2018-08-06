#include "types.h"
#include "terminal.h"


int kern_entry(){
    terminal_init();
    return 0;
}