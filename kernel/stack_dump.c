#include <printk.h>
#include "types.h"
#include "common.h"

void stack_dump(registers_ptr_t* registers) __attribute__((cdecl));

void stack_dump(registers_ptr_t* registers) {
    /*
     * ----- addr
     * ----- err_code
     * -----
     */

    uint32_t test_num = 1000;
    test_num++;
    printk("Oops! the kernel panic: %d\n", test_num);
    printk("int-%x, eip=%x, ds=%x, eflags=%x, eax=%x, ebx=%x, ecx=%x, edx=%x\n",
        registers->int_no, registers->eip, registers->ds, registers->eflags, registers->eax
            , registers->ebx, registers->ecx, registers->edx);
    hlt();
}