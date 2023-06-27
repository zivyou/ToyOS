file toyos_kernel
target remote localhost:1234
b kernel/intr/intr_s.S:100
b kernel/intr/intr_s.S:120
c