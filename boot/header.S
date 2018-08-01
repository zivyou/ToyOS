#include "header.h"

MBOOT_HEADER_MAGIC = 0x1BADB002
MBOOT_PAGE_ALIGN   = 1<<0
MBOOT_MEM_INFO     = 1<<1
MBOOT_HEAD_FLAG    = MBOOT_PAGE_ALIGN | MBOOT_MEM_INFO
MBOOT_CHECKSUM     = -(MBOOT_HEADER_MAGIC + MBOOT_HEAD_FLAG)



        .code32
        .section ".text", "ax"
        .long   MBOOT_HEADER_MAGIC
        .long   MBOOT_HEAD_FLAG
        .long   MBOOT_CHECKSUM
        .global _start
        .extern kern_entry
        .global glb_mboot_ptr

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
        
        
        /*movl    %ebx, (glb_mboot_ptr)*/
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
        jz      bs_die
        jmp     msg_loop

       

bs_die: 
        calll    kern_entry
        # invoke the BIOS reset code...
        hlt
        # Allow the user to press a key, then reboot
        xorw    %ax, %ax
        int     $0x16
        int     $0x19

        # int 0x19 should never return.  In case it does anyway,

        ljmp    $0xf000,$0xfff0
        
        
        .section ".data", "aw"
bugger_off_msg:
        .ascii  "Hello World!"
        .byte   0
        
        .section ".bss", "aw"
stack:
        .space 32768

STACK_TOP:


