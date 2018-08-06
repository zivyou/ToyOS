#include "types.h"



int kern_entry(){
    const uint32_t VGA_ADDR = (uint32_t )0xB8000;
    const uint8_t COLOR = (0 << 4) | (15 & 0x0F);
    const uint16_t blank = (uint16_t)COLOR << 8 | 0x20;
    
    uint16_t *dst = (uint16_t *)VGA_ADDR;
    uint32_t i=0;
    while (i < 0xFFFF){
        *(dst+i) = blank;
        i++;
    }
    
    
    
    char welcome[] = "Welcome to ToyOS kernel world!";
    i = 0;
    while(welcome[i]){
        dst[i] = (uint16_t)welcome[i] | (uint16_t)COLOR<<8;
        i++;
    }
    
    return 0;
}