# $@ = target file
# $< = first dependency
# $^ = all dependencies


QEMU-MEM = 4G
GDB = gdb
CFLAGS = -g -m32  -fno-pie -fno-builtin -fno-stack-protector -nostartfiles -nodefaultlibs \
		 -Wall -Wextra  -ffreestanding
#-Werror

CC = gcc
C_SOURCES = $(wildcard kernel/*.c drivers/*.c libc/*.c cpu/*.c utils/*.c rfs/*.c mem/*.c)
HEADERS = $(wildcard kernel/*.h drivers/*.h libc/*h cpu/*.h utils/*.h rfs/*.h mem/*.h)
# Nice syntax for file extension replacement
OBJ = ${C_SOURCES:.c=.o cpu/interrupt.o}

# First rule is the one executed when no parameters are fed to the Makefile

kernel/kernel.elf: boot/meminit.o ${OBJ} asm/functions.o
	nasm -f elf32 boot/multiboot_header.asm
	ld  -m elf_i386  -n -T linker.ld  -o $@  boot/multiboot_header.o  $^ --oformat=elf32-i386

kernel/kernel_entry.o: boot/meminit.asm asm/functions.asm
	nasm $< -f elf32 -o $@
	nasm $(word 2,$^) -f elf32 -o functions.o

kernel/kernel.o: kernel/kernel.c
	gcc ${CFLAGS} -c $< -o $@


debug-iso: os.iso kernel/kernel.elf
	qemu-system-i386 -m ${QEMU-MEM} -s -cdrom os.iso &
	${GDB} -ex "target remote localhost:1234" -ex "symbol-file kernel/kernel.elf"

clean:
	rm -f *.bin boot/*.bin boot/*.o kernel/*.o  kernel/*.elf kernel/*.bin drivers/*.o cpu/*.o libc/*.o mem/*.o  utils/*.o rfs/*.o *.iso  iso/boot/*.elf asm/*.o 

external: external/user_process.asm
	 nasm -f elf32  external/user_process.asm -o iso/boot/user_process


os.iso: kernel/kernel.elf
	cp $< iso/boot/
	grub-mkrescue -o os.iso iso

run-iso: os.iso
	qemu-system-i386 -m ${QEMU-MEM} -cdrom $<

# Generic rules for wildcards
# To make an object, always compile from its .c
%.o: %.c ${HEADERS}
	${CC} ${CFLAGS}  -c $< -o $@

%.o: %.asm
	nasm $< -f elf -o $@

%.bin: %.asm
	nasm $< -f bin -o $@




