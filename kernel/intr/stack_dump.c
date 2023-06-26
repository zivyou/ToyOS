#include "printk.h"
#include "types.h"
#include "common.h"

void stack_dump(registers_ptr_t* registers) __attribute__((cdecl));

void stack_dump(registers_ptr_t* registers) {
    /*
     * ----- addr
     * ----- err_code
     * -----
     */
    printk("Oops! the kernel panic: esp: 0x%x\n", registers->esp);
    printk("int-0x%x, cs=0x%x, eip=0x%x, ds=0x%x, eflags=0x%x, eax=0x%x, ebx=0x%x, ecx=0x%x, edx=0x%x, err_code=0x%x\n, \
        esp=0x%x,  ss=0x%x\
        ",
        registers->int_no, registers->cs, registers->eip, registers->ds, registers->eflags, registers->eax
            , registers->ebx, registers->ecx, registers->edx, registers->err_codes, registers->esp, registers->ss);
//    enable_interrput();
    while (1) hlt();
}