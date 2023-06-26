# eflags寄存器的布局

> https://wiki.osdev.org/CPU_Registers_x86#EFLAGS_Register

0: carry flag: 算术运算发生进位或借位
1： 保留位，永久为1
2： PF：parity flag： 奇偶标志位，用于表示运算结果中1的个数是奇数还是偶数；
3：保留位，通常为0；
4：AF：Auxiliary Carry Flag：辅助进位标志，借位运算用的；
5：保留位，通常为0；
6:ZR:zero flag: 指示运算结果是不是0
7：sign flag：指示运算结果是正数还是负数；
8：trap flag：单步调试开关
9：Interrupt enable flag：可屏蔽中断响应开关，1代表可响应，0代表不能响应；

14：NT:Nested task flag:嵌套任务标识，用来管理中断嵌套
17:VM:Virtual 8086 mode flag:设置为1时，开启虚拟8086的模式（用来伪装成8086,然后进入实模式运行）；关闭后回到保护模式；
