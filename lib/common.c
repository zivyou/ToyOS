
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

void outb(){
    
}