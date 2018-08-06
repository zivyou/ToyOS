#include "types.h"

enum vga_color {
    VGA_COLOR_BLACK = 0,
    VGA_COLOR_BLUE = 1,        
    VGA_COLOR_GREEN = 2,       
    VGA_COLOR_CYAN = 3,        
    VGA_COLOR_RED = 4,         
    VGA_COLOR_MAGENTA = 5,     
    VGA_COLOR_BROWN = 6,       
    VGA_COLOR_LIGHT_GREY = 7,  
    VGA_COLOR_DARK_GREY = 8,   
    VGA_COLOR_LIGHT_BLUE = 9,  
    VGA_COLOR_LIGHT_GREEN = 10,
    VGA_COLOR_LIGHT_CYAN = 11, 
    VGA_COLOR_LIGHT_RED = 12,  
    VGA_COLOR_LIGHT_MAGENTA = 13,   
    VGA_COLOR_LIGHT_BROWN = 14,
    VGA_COLOR_WHITE = 15,      
};
static const size_t VGA_WIDTH = 80;
static const size_t VGA_HEIGHT = 25;
typedef struct terminal{
    size_t cur_x;
    size_t cur_y;
    uint8_t terminal_color;
    uint16_t* terminal_buffer;
    size_t terminal_space;
    size_t buffer_point;
}terminal;

static terminal screen;

static inline uint16_t vga_entry(uint8_t data, uint8_t color){
    return ((uint16_t)data | (uint16_t)color<<8);
}

void terminal_init(){
    
    screen.cur_x = 0;
    screen.cur_y = 0;
    screen.terminal_color = VGA_COLOR_LIGHT_GREY | VGA_COLOR_BLACK << 4;
    screen.terminal_buffer = (uint16_t*)0xB8000;
    screen.buffer_point = 0;
    size_t x = 0;
    size_t y = 0;
    for (x=0; x<VGA_WIDTH; x++)
        for (y=0; y<VGA_HEIGHT; y++){
            screen.terminal_buffer[screen.buffer_point++] = vga_entry(' ', screen.terminal_color);
        }
}