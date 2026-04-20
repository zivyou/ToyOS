# 基本语法
__asm__ volatile ( "assembly code"
: output operands /* 可选 */
: input operands /* 可选 */
: list of clobbered registers /* 可选 */ );

# sample
__asm__ volatile (
    "movl %1, %%eax; movl %%eax, %0;"
    :"=r"(c) /* 输出 */
    :"r"(a) /* 输入 */
    :"%eax" /* 被修改的寄存器 */
)
%0, %1: 占位符; %0代表 =r(c)这一部分, %1代表r(a)这一部分;
=r代表由编译器选择一个通用寄存器来中转数据; 同理,如果=a代表强制由eax寄存器来中转;
最后一部分的%eax是在提醒编译器,这行代码执行的过程中可能会征用eax寄存器,让编译器优化的时候注意;