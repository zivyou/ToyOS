
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

void hlt(){
    __asm__ __volatile__ ("hlt");
}

void enable_interrput() {
    __asm__ __volatile__("sti");
}

void disable_interrupt() {
    __asm__ __volatile__("cli");
}