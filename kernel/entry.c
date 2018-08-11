#include "types.h"
#include "terminal.h"


int kern_entry(){
    terminal_init();
    int i=0;
    while(i < 8){
        terminal_print("hello world!\nhello world!!!!2");
        i++;
    }
    //terminal_scroll(-1);
    return 0;
}