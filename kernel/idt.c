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

void raw_set_idt_gate(int n, uint8_t flag, void *handler){
    uint32_t addr = (uint32_t) handler;
    idts[n].offset_lowerbits = addr & 0xFFFF;
    idts[n].offset_higherbits = (addr >> 16) & 0xFFFF;
    idts[n].flag = flag;
    idts[n].selector = 0x08;      // TSS中内核的CS在第8个
    idts[n].zero = 0;
    /*https://wiki.osdev.org/Interrupts_tutorial*/
}



void idt_init(){
    idts_ptr.base = (uint32_t)&idts;
    idts_ptr.limit = (uint16_t)(sizeof(struct idt_entry) * 256 - 1);

#define set_idt_gate(index, flag) \
    do {                          \
        extern void isr_##index();                         \
        raw_set_idt_gate(index, flag, isr_##index);        \
    } while (0);
    // setup isr

    set_idt_gate(0, 0x8E);
    set_idt_gate(1, 0x8E);
    set_idt_gate(2, 0x8E);
    set_idt_gate(3, 0x8E);
    set_idt_gate(4, 0x8E);
    set_idt_gate(5, 0x8E);
    set_idt_gate(6, 0x8E);
    set_idt_gate(7, 0x8E);
    set_idt_gate(8, 0x8E);
    set_idt_gate(9, 0x8E);
    set_idt_gate(10, 0x8E);
    set_idt_gate(11, 0x8E);
    set_idt_gate(12, 0x8E);
    set_idt_gate(13, 0x8E);
    set_idt_gate(14, 0x8E);
    set_idt_gate(15, 0x8E);
    set_idt_gate(16, 0x8E);
    set_idt_gate(17, 0x8E);
    set_idt_gate(18, 0x8E);
    set_idt_gate(19, 0x8E);
    set_idt_gate(20, 0x8E);
    set_idt_gate(21, 0x8E);
    set_idt_gate(22, 0x8E);
    set_idt_gate(23, 0x8E);
    set_idt_gate(24, 0x8E);
    set_idt_gate(25, 0x8E);
    set_idt_gate(26, 0x8E);
    set_idt_gate(27, 0x8E);
    set_idt_gate(28, 0x8E);
    set_idt_gate(29, 0x8E);
    set_idt_gate(30, 0x8E);
    set_idt_gate(31, 0x8E);

    // ...
    set_idt_gate(255, 0x8E);
    __asm__ volatile ("lidt %0"::"m"(idts_ptr));
    __asm__ volatile ("sti");
}