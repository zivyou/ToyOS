
C_FILES=$(shell find -name "*.c")
S_FILES=$(shell find -name "*.S")
C_OBJECT=$(patsubst %.c, %.o, ${C_FILES})
S_OBJECT=$(patsubst %.S, %.o, ${S_FILES})



INCLUDE_DIR=include

GCC_C_OPTION=-m32 -c -nostdlib -nostdinc -fno-stack-protector -I ${INCLUDE_DIR} -Wall -fno-builtin -O0
GCC_S_OPTION=--32 -Wall -I ${INCLUDE_DIR} 
LD_OPTION=-T setup.ld -m elf_i386 -nostdlib

KERN_NAME = toyos_kernel

CC=gcc
LD=ld
AS=as

all:  ${C_OBJECT} $(S_OBJECT) link update_image

.c.o:
	${CC} ${GCC_C_OPTION} $< -o $@ 
.S.o:
	${AS} ${GCC_S_OPTION} $< -o $@

link: 
	${LD} ${LD_OPTION} ${C_OBJECT} ${S_OBJECT} -o ${KERN_NAME}
	
.PHONY:update_image
update_image:
	sudo mount floppy.img /mnt/kernel
	sudo cp $(KERN_NAME) /mnt/kernel/$(KERN_NAME)
	sleep 1
	sudo umount /mnt/kernel

.PHONY:mount_image
mount_image:
	sudo mount floppy.img /mnt/kernel

.PHONY:umount_image
umount_image:
	sudo umount /mnt/kernel

.PHONY:qemu
qemu:
	qemu-system-i386 -fda floppy.img -boot a

.PHONY:copy
copy:
	cp ${KERN_NAME} /boot/
	cp initrd /boot/
clean:
	rm ${S_OBJECT}
	rm ${C_OBJECT}
	rm ${KERN_NAME}
