
C_FILES=$(shell find -name "*.c")
S_FILES=$(shell find -name "*.S")
C_OBJECT=$(patsubst %.c, %.o, ${C_FILES})
S_OBJECT=$(patsubst %.S, %.o, ${S_FILES})



INCLUDE_DIR=include

GCC_C_OPTION=-m32 -c -g -nostdlib -nostdinc -fno-stack-protector -I ${INCLUDE_DIR} -Werror -fno-builtin -O0
GCC_S_OPTION=--32 -g --gstabs -I ${INCLUDE_DIR} --warn
LD_OPTION=-T setup.ld -m elf_i386 -nostdlib

KERN_NAME = toyos_kernel

CC=gcc
LD=ld
AS=as

all:  $(S_OBJECT) ${C_OBJECT}  link iso

.S.o:
	${AS} ${GCC_S_OPTION} $< -o $@

.c.o:
	${CC} ${GCC_C_OPTION} $< -o $@ 


link: 
	${LD} ${LD_OPTION} ${C_OBJECT} ${S_OBJECT} -o ${KERN_NAME}

.PHONY:qemu
qemu:
	## qemu-system-i386  -s -monitor stdio -fda floppy.img -boot a
	## qemu-system-i386  -s -monitor stdio -cdrom toyos_kernel.iso -boot a
	qemu-system-i386  -s -monitor stdio -kernel toyos_kernel -boot a
check_boot:
	grub-file --is-x86-multiboot toyos_kernel
	cp toyos_kernel iso/boot/

iso: check_boot link
	grub-mkrescue -o toyos_kernel.iso ./iso

iso_qemu:
	qemu-system-i386  -s -monitor stdio -cdrom toyos_kernel.iso -boot a

clean:
	rm ${S_OBJECT}
	rm ${C_OBJECT}
	rm ${KERN_NAME}
