#include <types.h>
#include <common.h>


/**
 *                ------------------------> x
 *                |
 *                |
 *                |
 *                |
 *                |
 *                |
 *                V y
 */

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
    size_t cursor_pos;
}terminal;

static terminal screen;

static inline uint16_t vga_entry(uint8_t data, uint8_t color){
    return ((uint16_t)data | (uint16_t)color<<8);
}

static int screen_full(){
    if (/*screen.cur_x > VGA_WIDTH-1 &&*/ screen.cur_y>VGA_HEIGHT-1){
        return 1;
    }
    return 0;
}

void terminal_init(){
    
    screen.cur_x = 0; //[0, VGA_WIDTH)
    screen.cur_y = 0; //[0, VGA_HEIGHT)
    screen.terminal_color = VGA_COLOR_LIGHT_GREY | VGA_COLOR_BLACK << 4;
    screen.terminal_buffer = (uint16_t*)0xB8000;
    screen.cursor_pos = 0;
    size_t x = 0;
    size_t y = 0;
    for (x=0; x<VGA_WIDTH; x++)
        for (y=0; y<VGA_HEIGHT; y++){
            screen.terminal_buffer[screen.cursor_pos++] = vga_entry(' ', screen.terminal_color);
        }
}

static void terminal_scroll(int l){
    if (l < 0){
        /* scroll up l line: remove top l lines from screen */
        uint16_t *temp_buffer = screen.terminal_buffer + (-l)*VGA_WIDTH;
        int i = (-l)*VGA_WIDTH;
        int j=0;
        while (i < VGA_HEIGHT*VGA_WIDTH){
            screen.terminal_buffer[j] = temp_buffer[i];
            j++;
            i++;
        }
        for (; j<VGA_HEIGHT*VGA_WIDTH; j++){
            screen.terminal_buffer[j] = vga_entry(' ', screen.terminal_color);
        }
        screen.cur_y = VGA_HEIGHT + l;
        screen.cur_x = 0;
    }else{
        /* emm.. seems scrolling down is not reasonable.. */
    }
}

static void move_cursor(uint16_t x, uint16_t y){
    screen.cursor_pos = y*VGA_WIDTH+x;
    outb(0x3D4, 14);                        
    outb(0x3D5, (screen.cursor_pos >> 8)&0xFF);
    outb(0x3D4, 15);
    outb(0x3D5, (screen.cursor_pos)&0xFF);
    
}

static void terminal_put_char(char c){
    if (c == '\r'){
        screen.cur_x = 0;
    }else if (c == '\n'){
        screen.cur_x = 0;
        screen.cur_y++;
    }else if (c == 0x08){
        screen.cur_x--;
    }else{
        screen.terminal_buffer[screen.cur_y*VGA_WIDTH+screen.cur_x] = (screen.terminal_color<<8) | (uint16_t)c;
        screen.cur_x++;
    }
    
    if (screen.cur_x >= VGA_WIDTH){
        screen.cur_x = 0;
        screen.cur_y ++;
    }
    
    if (screen.cur_x < 0){
        if (screen.cur_y > 0){
            screen.cur_x = VGA_WIDTH;
            screen.cur_y --;
        }else{
            screen.cur_x = screen.cur_y = 0;
        }
    }
    if (screen_full()){
        /* the scree is full, need to scroll */
         terminal_scroll(-1);
    }
    move_cursor(screen.cur_x, screen.cur_y);
}

void terminal_print(char *str){
    char *s = str;
    while (*s){
        terminal_put_char(*s);
        s++;
    }
}