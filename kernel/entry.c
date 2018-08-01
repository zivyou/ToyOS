#include "types.h"



int kern_entry(){
    const uint32_t VGA_ADDR = (uint32_t )0xB8000;
    const uint8_t COLOR = 0x61;
    
    uint8_t *dst = (uint8_t *)VGA_ADDR;
    uint32_t i=0;
    while (i < 0xFFFF){
        *(dst+i) = 0;
        i++;
    }
    
    char welcome[] = "Welcome to ToyOS kernel world!";
    i = 0;
    while(welcome[i]){
        uint32_t index = (uint32_t)(i*2);
        /* This really confused me: if I write like this: *(dst+(uint32_t)(i*2)) = welcome[i];
         * the code just doesnot work. What the fucking problem with gcc? 
         */
        *(dst+(index)) = welcome[i];
        *(dst+index+1) = COLOR;
        i++;
    }
    
    return 0;
}