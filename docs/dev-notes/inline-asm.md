# C内联汇编

1. 基本格式为： (指令:输出:输入)
2. 制定输出的数据来源于哪个寄存器：如果是来自于eax, 则输出部分为：  =a(var)
3. 输入操作数的一些修饰符：
    - r: 告诉gcc编译器可以自己选择一个合适的寄存器来传输输入值