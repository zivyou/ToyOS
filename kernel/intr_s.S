
    .code32
    .extern stack_dump

.altmacro
.macro ISR_NOERRCODE num
    .global isr_\num
isr_\num&:
    pushl $0
    pushl $\num
    jmp common_isr
.endm

.altmacro
.macro ISR_ERRCODE num
    .global isr_\num
isr_\num&:
    pushl $\num
    jmp common_isr
.endm

.altmacro
.macro IRQ_HANDLES num
    .global irq_handler_\num
irq_handler_\num&:
    pushl $\num
    jmp irq_common_stub
.endm


common_isr:
    pushl %eax
    pushl %ecx
    pushl %edx
    pushl %ebx
    pushl %esp
    pushl %ebp
    pushl %esi
    push  $0
    push  %ds

    pushl %esp
    call stack_dump
    addl $4, %esp

    popl %ebx
    mov %bx, %ds
    mov %bx, %ss
    mov %bx, %es
    mov %bx, %fs
    mov %bx, %gs

    popl %esi
    popl %ebp
    popl %esp
    popl %ebx
    popl %edx
    popl %ecx
    popl %eax
    addl $8, %esp # 清理err_codes, int_no
    iret

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
    mov %dx, %gs
    mov %dx, %ss

    call stack_dump


    addl $8, %esp # remove these two params of stack_dump()
    popl %fs
    popl %es
    popl %ds
    mov %ds, %dx
    mov %dx, %gs
    mov %dx, %ss
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
    mov %dx, %gs
    mov %dx, %ss

    call stack_dump


    addl $8, %esp  # remove these two params of stack_dump()
    popl %fs
    popl %es
    popl %ds
    mov %ds, %dx
    mov %dx, %gs
    mov %dx, %ss
    popl %ebp
    popl %edi
    popl %esi
    popl %esi
    popl %edx
    popl %ecx
    popl %ebx
    popl %eax
    iret

ISR_ERRCODE 8   # 双重故障
ISR_ERRCODE 10  # 无效TSS
ISR_ERRCODE 11  # 段不存在
ISR_ERRCODE 12  # 栈错误
ISR_ERRCODE 13  # 常规保护
ISR_ERRCODE 14  # 页故障
ISR_ERRCODE 17  # 对齐检查

ISR_NOERRCODE 0  # 除0异常
ISR_NOERRCODE 1  # 调试异常
ISR_NOERRCODE 2
ISR_NOERRCODE 3  # 断点异常
ISR_NOERRCODE 4  # 一簇合
ISR_NOERRCODE 5  # 对数组的引用超出边界
ISR_NOERRCODE 6  # 无效的操作码
ISR_NOERRCODE 7  # 设备不可用
ISR_NOERRCODE 9  # 协处理器跨段
ISR_NOERRCODE 15 # 保留
ISR_NOERRCODE 16 # 浮点处理单元错误
ISR_NOERRCODE 18 # 机器检查
ISR_NOERRCODE 19 # 浮点异常
# CPU保留
ISR_NOERRCODE 20
ISR_NOERRCODE 21
ISR_NOERRCODE 22
ISR_NOERRCODE 23
ISR_NOERRCODE 24
ISR_NOERRCODE 25
ISR_NOERRCODE 26
ISR_NOERRCODE 27
ISR_NOERRCODE 28
ISR_NOERRCODE 29
ISR_NOERRCODE 30
ISR_NOERRCODE 31
# 系统调用
ISR_NOERRCODE 255

    .extern irq_handler
irq_common_stub:
    pushal
    pushl %ds
    pushl %es
    pushl %fs

    mov %ds, %ax
    push %ax
    push $0

    mov $0x10, %ax
    mov %ax, %ds
    mov %ax, %es
    mov %ax, %fs
    mov %ax, %gs
    mov %ax, %ss

    movl 48(%esp), %eax
    pushl %eax
    call irq_handler
    addl $4, %esp

    pop %bx
    pop %bx
    mov %bx, %ds
    popl %fs
    popl %es
    popl %ds
    popal
    addl $8, %esp
    iret



IRQ_HANDLES 32
IRQ_HANDLES 33
IRQ_HANDLES 34
IRQ_HANDLES 35
IRQ_HANDLES 36
IRQ_HANDLES 37
IRQ_HANDLES 38
IRQ_HANDLES 39
IRQ_HANDLES 40
IRQ_HANDLES 41
IRQ_HANDLES 42
IRQ_HANDLES 43
IRQ_HANDLES 44
IRQ_HANDLES 45
IRQ_HANDLES 46
IRQ_HANDLES 47
