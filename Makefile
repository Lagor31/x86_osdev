# $@ = target file
# $< = first dependency
# $^ = all dependencies


QEMU-MEM = 128M
GDB = gdb
CFLAGS = -g -m32  -fno-pie -fno-builtin -fno-stack-protector -nostartfiles -nodefaultlibs \
		 -Wall -Wextra  -ffreestanding
#-Werror

CC = gcc
C_SOURCES = $(wildcard kernel/*.c users/*.c drivers/*.c  lib/*.c cpu/*.c mem/*.c proc/*.c lock/*.c bin/*.c)
HEADERS = $(wildcard kernel/*.h drivers/*.h users/*.h lib/*h cpu/*.h mem/*.h proc/*.h lock/*.h bin/*.h)
# Nice syntax for file extension replacement
OBJ = ${C_SOURCES:.c=.o cpu/interrupt.o asm/functions.o}

# First rule is the one executed when no parameters are fed to the Makefile

kernel/kernel.elf: boot/meminit.o ${OBJ} 
	make process
	nasm -f elf32 boot/multiboot_header.asm
	ld  -m elf_i386  -n -T linker.ld  -o $@  boot/multiboot_header.o  $^ --oformat=elf32-i386

kernel/kernel_entry.o: boot/meminit.asm asm/functions.asm 
	nasm $< -f elf32 -o $@
	nasm $(word 2,$^) -f elf32 -o functions.o

kernel/kernel.o: kernel/kernel.c
	gcc ${CFLAGS} -c $< -o $@


debug-iso: os.iso kernel/kernel.elf
	qemu-system-i386 -d cpu_reset  -m ${QEMU-MEM} -s -cdrom os.iso &
	${GDB} -ex "target remote localhost:1234" -ex "symbol-file kernel/kernel.elf"

clean:
	rm -f *.bin boot/*.bin boot/*.o kernel/*.o bin/*.o kernel/*.elf kernel/*.bin drivers/*.o proc/*.o cpu/*.o lib/*.o mem/*.o  *.iso  iso/boot/*.elf asm/*.o lock/*.o external/*.o external/*.out

process:
	nasm external/syscalls.asm -f elf32 -o external/syscalls.o
	${CC} ${CFLAGS} -c external/prog.c -o external/prog.o
	${CC} ${CFLAGS} -c external/libc.c -o external/libc.o

	ld -m elf_i386 external/prog.o external/libc.o external/syscalls.o -o external/user_program.out --oformat=elf32-i386



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




