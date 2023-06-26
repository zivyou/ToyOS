MBOOT_HEADER_MAGIC = 0x1BADB002
MBOOT_PAGE_ALIGN   = 1<<0
MBOOT_MEM_INFO     = 1<<1
MBOOT_HEAD_FLAG    = MBOOT_PAGE_ALIGN | MBOOT_MEM_INFO
MBOOT_CHECKSUM     = -(MBOOT_HEADER_MAGIC + MBOOT_HEAD_FLAG)



        .code32
        .section ".multiboot"
        .align  0x4
        .long   MBOOT_HEADER_MAGIC
        .long   MBOOT_HEAD_FLAG
        .long   MBOOT_CHECKSUM

        .section ".init.text", "ax"
        .align 16
        .global _start
        .extern kern_entry
        .extern gdt_init
        .extern gp

_start:
        movl    $STACK_TOP, %ebp
        movl    %ebp, %esp
        andl    $0x0FFFFFFF0, %esp

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
        /**
        开启A20地址线，来获取更大的内存地址可访问空间
        */
        inb     $0x92, %al
        or      $0x10, %al
        outb    %al,   $0x92

        /**
        开启保护模式
        */
        movl    %cr0, %eax
        or    $1,   %eax
        movl    %eax, %cr0
1:
        
        movw         $0x0010,  %ax
        movw         %ax,    %ds
        movw         %ax,    %es
        movw         %ax,    %fs
        movw         %ax,    %gs
        movw         %ax,    %ss
        
        cli

        call    kern_entry
        # invoke the BIOS reset code...
        hlt

        
        
        
        .section ".init.data", "aw"
        .align  0x4
bugger_off_msg:
        .ascii  "Hello World!"
        .byte   0

        .section ".init.bss", "aw"
        .align  0x4
stack:
        .space 65536

STACK_TOP:


