#include "types.h"
#include "printk.h"
#include "common.h"

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

typedef void (*intr_handler_func) (registers_ptr_t*);
static intr_handler_func handlers[256] __attribute__((aligned(4)));

#define IO_PIC1   (0x20)    // Master (IRQs 0-7)
#define IO_PIC2   (0xA0)    // Slave  (IRQs 8-15)

#define IO_PIC1C  (IO_PIC1+1)
#define IO_PIC2C  (IO_PIC2+1)

void raw_set_idt_gate(int n, uint8_t flag, void *handler){
    if (n>44)
    printk("setting up idt gate......%d\n", n);
    uint32_t addr = (uint32_t) handler;
    idts[n].offset_lowerbits = addr & 0xFFFF;
    idts[n].offset_higherbits = (addr >> 16) & 0xFFFF;
    idts[n].flag = flag;
    idts[n].selector = 0x08;      // TSS中内核的CS在第8个
    idts[n].zero = 0;
    /*https://wiki.osdev.org/Interrupts_tutorial*/
}

void register_irq_handler(int32_t irq_num, intr_handler_func handler) {
    handlers[irq_num] = handler;
}

void clock_callback(registers_ptr_t* registers) {
    printk("==============================>");
}

void init_interrupt_chip() {
    // 设置 8259A 芯片
// 重新映射 IRQ 表
        // 两片级联的 Intel 8259A 芯片
        // 主片端口 0x20 0x21
        // 从片端口 0xA0 0xA1
        // 初始化主片、从片
        // 0001 0001
        outb(IO_PIC1, 0x11);
        outb(IO_PIC2, 0x11);

        // 设置主片 IRQ 从 0x20(32) 号中断开始
        outb(IO_PIC1C, 0x20);

        // 设置从片 IRQ 从 0x28(40) 号中断开始
        outb(IO_PIC2C, 0x28);

        // 设置主片 IR2 引脚连接从片
        outb(IO_PIC1C, 0x04);

        // 告诉从片输出引脚和主片 IR2 号相连
        outb(IO_PIC2C, 0x02);

        // 设置主片和从片按照 8086 的方式工作
        outb(IO_PIC1C, 0x01);
        outb(IO_PIC2C, 0x01);

        // 设置主从片允许中断
        outb(IO_PIC1C, 0x0);
        outb(IO_PIC2C, 0x0);
}

void idt_init(){
    init_interrupt_chip();
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

    // 系统调用
     set_idt_gate(255, 0x8E);
    // set_irq_handler
#define set_irq_handle(index, flag) \
    do {                          \
        extern void irq_handler_##index();                         \
        raw_set_idt_gate(index, flag, irq_handler_##index);        \
    } while (0);

    set_irq_handle(32, 0x8E);
    set_irq_handle(33, 0x8E);
    set_irq_handle(34, 0x8E);
    set_irq_handle(35, 0x8E);
    set_irq_handle(36, 0x8E);
    set_irq_handle(37, 0x8E);
    set_irq_handle(38, 0x8E);
    set_irq_handle(39, 0x8E);
    set_irq_handle(40, 0x8E);
    set_irq_handle(41, 0x8E);
    set_irq_handle(42, 0x8E);
    set_irq_handle(43, 0x8E);
    set_irq_handle(44, 0x8E);
    set_irq_handle(45, 0x8E);
    set_irq_handle(46, 0x8E);
    set_irq_handle(47, 0x8E);

    for (int i=32; i<48; i++)
        register_irq_handler(i, clock_callback);
    __asm__ volatile ("lidt %0"::"m"(idts_ptr));
    __asm__ volatile ("sti");
}

void irq_handler(registers_ptr_t* registers) {
    printk("handling irq.....");


    if (registers->int_no >= 40) {
        outb(IO_PIC2, 0x20);
    }
    outb(IO_PIC1, 0x20);

    printk("handling irq.....");
    if (handlers[registers->int_no] != 0) {
         handlers[registers->int_no](registers);
    }
    enable_interrput();
    while (1) hlt();
}

