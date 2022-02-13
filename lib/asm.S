    .code32
    .align  0x4
    .global     gdt_flush
    .section .text
    
    
gdt_flush:
    cli
    movl        4(%esp),       %eax
    lgdt        (%eax)

    mov $0x10,  %ax
    mov %ax,    %ds
    mov %ax,    %es
    mov %ax,    %fs
    mov %ax,    %gs
    # mov %ax,    %ss # 测试时发现直接给SS寄存器赋值会导致所有的寄存器数值混沦，不清楚是不是qemu的CPU特性
    jmp $0x08,  $flush

flush:
    ret
    

    
