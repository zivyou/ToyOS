    .code32
    .align  0x4
    .global     gdt_flush
    
    
gdt_flush:
    
    movl        4(%esp),       %eax
    lgdt        (%eax)

    
    ret
    

    
