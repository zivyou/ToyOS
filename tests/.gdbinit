file toyos_kernel
target remote localhost:1234
b boot/header.S:29
b kernel/mm/mm.c:133
c