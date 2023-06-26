# gdb的一些不太常用指令

1. 展示断点列表： 

2. 查看栈帧:
  - bt
3. 选择栈帧：
  - frame ${n}
  - 选择完了之后使用info frame查看栈帧全部信息

4. gdb启动时制定init文件：  gdb -x ${file}

5. gdb打印从一片内存区域的值(以打印栈为例)：
  - x /nxu *(int*)($esp)
  - 其中，n为个数，u为单元。例如，32bit机器，打印栈上10个元素，那就是 x /10xw *(int*)($esp)