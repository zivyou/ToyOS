#include <types.h>

typedef struct idt_entry{
    /* 8 bytes for each entry */
    
    uint16_t    offset_lowerbits;
    uint16_t    selector; /* kernel code segment offset */
    uint8_t     zero;
    /*
    uint8_t     type: 4；
    uint8_t     always0: 1;
    uint8_t     dpl: 2;   // Descriptor Privilege Level: describe if user-space process can access gate entry 
    uint8_t     present: 1;
    */
    uint8_t     flag;
    uint16_t    offset_higherbits;
} __attribute__((packed)) idt_entry;

typedef struct idts_ptr_t{
    uint16_t    limit;
    uint32_t    base;
} __attribute__((packed)) idts_ptr_t;

struct idt_entry idts[256] = {0, };
struct idts_ptr_t idts_ptr;

void set_idt_gate(int n, uint8_t flag, void *handler){
    uint32_t addr = (uint32_t) handler;
    idts[n].offset_lowerbits = addr & 0xFFFF;
    idts[n].offset_higherbits = (addr >> 16) & 0xFFFF;
    idts[n].flag = flag;
    
    /*https://wiki.osdev.org/Interrupts_tutorial*/
}

void idt_init(){
    idts_ptr.base = (uint32_t)&idts;
    idts_ptr.limit = (uint16_t)(sizeof(struct idt_entry) * 256 - 1);
}