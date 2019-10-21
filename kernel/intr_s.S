
    .code32
    .extern stack_dump

.altmacro
.macro _INT_NOERRCODE num
    .global int_\num
int_\num&:
    jmp no_err_code
.endm


_INT_NOERRCODE 0
_INT_NOERRCODE 1


.altmacro
.macro _INT_ERRCODE num 
    .global int_\num
int_\num&:
    pushl $\num
    jmp err_code
.endm

_INT_ERRCODE 128

no_err_code:
    # pushl %eax
    xchgl %eax, (%esp)
    pushl %ebx
    pushl %ecx
    pushl %edx
    pushl %esi
    pushl %edi
    pushl %ebp
    pushl %ds
    pushl %es
    pushl %fs
    pushl %eax
    leal 48(%esp), %edx
    pushl %edx

    mov $0x10, %dx
    mov %dx, %ds
    mov %dx, %es
    mov %dx, %fs

    call stack_dump


    addl $8, %esp # remove these two params of stack_dump()
    popl %fs
    popl %es
    popl %ds
    popl %ebp
    popl %edi
    popl %esi
    popl %esi
    popl %edx
    popl %ecx
    popl %ebx
    popl %eax
    iret

err_code:
    pushl %eax
    pushl %ebx
    pushl %ecx
    pushl %edx
    pushl %esi
    pushl %edi
    pushl %ebp
    pushl %ds
    pushl %es
    pushl %fs
    pushl $0
    lea  48(%esp), %edx
    push %edx

    mov $0x10, %dx
    mov %dx, %ds
    mov %dx, %es
    mov %dx, %fs

    call stack_dump


    addl $8, %esp  # remove these two params of stack_dump()
    popl %fs
    popl %es
    popl %ds
    popl %ebp
    popl %edi
    popl %esi
    popl %esi
    popl %edx
    popl %ecx
    popl %ebx
    popl %eax
    iret

