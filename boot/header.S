#include "header.h"

MBOOT_HEADER_MAGIC = 0x1BADB002
MBOOT_PAGE_ALIGN   = 1<<0
MBOOT_MEM_INFO     = 1<<1
MBOOT_HEAD_FLAG    = MBOOT_PAGE_ALIGN | MBOOT_MEM_INFO
MBOOT_CHECKSUM     = -(MBOOT_HEADER_MAGIC + MBOOT_HEAD_FLAG)



        .code32
        .align  0x4
        .section ".text", "ax"
        .long   MBOOT_HEADER_MAGIC
        .long   MBOOT_HEAD_FLAG
        .long   MBOOT_CHECKSUM
        .global _start
        .extern kern_entry
        .extern gdt_init
        .extern gp

_start:
        movl    $STACK_TOP, %esp
VGA_ADDR        = 0xB8000
COLOR           = 0x61

        movl    $0xFFFF, %ecx
        movl    $VGA_ADDR, %edi
clear:
        movl    $0, (%edi)
        inc     %edi
        dec     %ecx
        jnz     clear
        
        
        xorl    %edi, %edi
        xorl    %esi, %esi
        xorl    %eax, %eax
        xorl    %ebx, %ebx
        
        movl    $bugger_off_msg, %esi

        movl    $VGA_ADDR, %edi

msg_loop:

        lodsb

        movb    %al, (%edi)
        inc     %edi
        movl    $COLOR, (%edi)
        inc     %edi
        andb    %al, %al
        jz      entry
        jmp     msg_loop
                
        
entry:  

        movl    %cr0, %eax
        or      $1,   %eax
        movl    %eax, %cr0
        
        ljmp    $0x08, $1f
1:
        
        movw         $0x0010,  %ax
        movw         %ax,    %ds
        movw         %ax,    %es
        movw         %ax,    %fs
        movw         %ax,    %gs
        movw         %ax,    %ss
        
        cli
        /*
        8259A重新编程。
        在intel的设计中，为硬件中断保留的中断号为int 0x20-0x2F.但是BIOS没有遵从这个设定，BIOS的硬件中断号
        设定为0x08-0x0F。因此这里需要重新设定8259A芯片。
        下面这段代码是从linux-0.12中copy过来的，其中涉及到8259A芯片的电路涉及原理，包括级联、上升沿触发方式、
        CPU时钟周期等，实在太过晦涩。
        .word   0x00eb 是一条指令，这条指令的作用仅仅是让IP寄存器+2. 之所以加这么一条看似无意义的指令，是
        在等CPU的时钟周期。因为8259A级联CPU在一个CPU时钟周期内无法完成置位操作。
        */

        mov     $0x11, %al
        out     %al, $0x20
        .word   0x00eb, 0x00eb
        out     %al, $0xa0
        .word   0x00eb, 0x00eb

        mov     $0x20, %al
        out     %al, $0x21
        .word   0x00eb, 0x00eb
        mov     $0x28, %al
        out     %al, $0xa1
        .word   0x00eb, 0x00eb
        mov     $0x04, %al
        out     %al, $0x21

        .word   0x00eb, 0x00eb
        mov     $0x02, %al
        out     %al, $0xa1

        .word   0x00eb, 0x00eb
        mov     $0x01, %al
        out     %al, $0x21

        .word   0x00eb, 0x00eb
        out     %al, $0xa1
        .word   0x00eb, 0x00eb
        mov     $0xff, %al
        out     %al, $0x21
        .word   0x00eb, 0x00eb
        out     %al, $0xa1

        calll    kern_entry
        # invoke the BIOS reset code...
        hlt

        
        
        
        .section ".data", "aw"
        .align  0x4
bugger_off_msg:
        .ascii  "Hello World!"
        .byte   0
        
        .section ".bss", "aw"
        .align  0x4
stack:
        .space 32768

STACK_TOP:


