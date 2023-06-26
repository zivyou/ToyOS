
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
    pushl $0
    pushl $\num
    jmp common_irq
.endm


common_isr:
    cli
    pusha

    movl %ds, %eax
    pushl %eax

    mov $0x10, %ax
    mov %ax, %ds
    mov %ax, %ss
    mov %ax, %es
    mov %ax, %fs
    mov %ax, %gs

    pushl %esp
    call stack_dump
    addl $4, %esp

    popl %ebx
    mov %bx, %ds
    mov %bx, %ss
    mov %bx, %es
    mov %bx, %fs
    mov %bx, %gs

    popa
    addl $8, %esp # 清理err_codes, int_no
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

    .extern irq_handle
common_irq:
    pusha
    mov $0x10, %ax
    mov %ax, %ds
    mov %ax, %ss
    mov %ax, %es
    mov %ax, %fs
    mov %ax, %gs

    pushl %esp
    /**
    call 指令会自动将eip寄存器压入栈, call指令等价于：
       sub $4, %esp
       movl %eip, (%esp) 
       jmp function
    ret指令会先弹出一个值到eip，然后跳转到eip的地方继续执行
    而iret指令是interrupt return，会在跳转前自动做一些事情： (1)实模式下，从栈中恢复eip,cs和eflags；(2)保护模式下，依据eflags寄存器的内容，采取不同的返回策略；
    保护模式下主要看eflags的14号和17号位的值，但是与我们遇到的问题并没有什么直接的关系。因为经调试，出问题的时候这两个值都是0.
    那么，都是0的情况下，iret的行为如下：

    */
    call irq_handle
    addl $4, %esp

    mov $0x10, %bx
    mov %bx, %ds
    mov %bx, %ss
    mov %bx, %es
    mov %bx, %fs
    mov %bx, %gs

    popa
    /**
    上面的irq_handler_宏定义里我们压入了一个num，要记得弹出来。同时，irq系统都会自动帮我们压入一个errno（隐式压入的），因此我们需要弹8个字节出来
    */
    addl $8, %esp
    /**
    https://stackoverflow.com/questions/10581224/why-does-iret-from-a-page-fault-handler-generate-interrupt-13-general-protectio
    */
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


// 有一些个中断号应该干什么事情，是CPU定义的，咱没得选。
/**
0-31 是被CPU厂商抢去用了； 
32-47号分配给了可屏蔽硬件中断，就是说在发生这些硬件中断的时候，CPU可以选择不响应；为啥CPU可以这么横呢？这是因为32-47号中断是连接到了一块8259A芯片上，CPU可以选择不理这块芯片的信号；
48-255号中断向量没人管，想怎么整都行。

32-255号中断向量是写代码的人可以自己选择分配怎么使用的，由于咱也不会做系统设计，因此就直接抄Linux的作业，Linux怎么分配咱就怎么分配。

Linux内核是这么搞的： （这部分没看linux源码，下面的内容是从《understanding linux kernel》上抄的）
1. 32号分配给时钟中断（8253芯片发出的100Hz时钟中断）；
2. 33号分配给键盘；
3. 34号分配给级联8259A芯片（一个电脑会用两块8259A芯片去级联，这条线就是去第二块芯片的）；
4. 35号分配给第二串口；
5. 36号分配给第一串口；
6. 37号分配给第二并行口；
7. 38号分配给软盘；
8. 39号分配给第一并行口
9. 40号分配给系统时钟（jiffies，一个摩托罗拉的MC146818芯片，有自己的纽扣电池，可以想秒表一样掐表计时。频率一般在2-8192Hz）
10. 41保留、
11. 42保留
12. 43保留
13. 44鼠标
14. 45数字协处理器
15. 46硬盘
16. 47保留
*/

/**
32号和40号确实都是与时钟相关的中断：
这两实现的时候采用的芯片不一样，32号是8253芯片，这款芯片是一款可编程芯片，在上电后初始化一下，就可以让它定时戳我们一下。
MC146818芯片自己带电池，更像是电脑自带的一个表, x86下端口地址一般为0x40,0x41,0x42,0x43
*/