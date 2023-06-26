# 一些常用的qemu指令
> 测试全靠这些了

1. 用qemu控制台打印寄存器值
    - info registers
    - print $eax
2. 打印内存中的数据：
    - xp /1x 0x123556

3. 将qemu控制台打印的内容保存到文件