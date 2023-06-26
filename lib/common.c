
/*
 * 
 * __asm__ [volatile] [goto] (AssemblerTemplate
                           [ : OutputOperands ]
                           [ : InputOperands  ]
                           [ : Clobbers       ]
                           [ : GotoLabels     ]);
 * 
 
    + - an operand is read and written by an instruction;
    & - output register shouldn't overlap an input register and should be used only for output;
    % - tells the compiler that operands may be commutative.


 */
#include <common.h>
void outb(uint16_t addr, uint8_t data){
    /* N here stands for immediate num */
    __asm__ __volatile__ ("outb %1, %0"::"dN"(addr), "a"(data));
}


uint8_t inb(uint16_t addr){
    uint8_t data;
    __asm__ __volatile__ ("inb %1, %0":"=r"(data):"dN"(addr));
    return data;
}

/**
 * "hlt" operand set CPU into HALT state.
 * CPU在HALT状态下关闭一些功能来节约功耗，但是在此期间能够响应中断，因此一般用来设置成待机模式； 
*/
void hlt(){
    __asm__ __volatile__ ("hlt");
}

void enable_interrput() {
    /**
     * sti指令会设置eflags寄存器中IF标志位的值，设置上了之后CPU就会响应可屏蔽硬件中断了。
     * 
    */
    __asm__ __volatile__("sti");
}

void disable_interrupt() {
    __asm__ __volatile__("cli");
}