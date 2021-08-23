#ifndef ELF__H
#define ELF__H

#define USER_STACK_ADDR 0x04000000

#include "../cpu/types.h"
#include "../drivers/screen.h"
#include "../proc/thread.h"
#include <elf.h>

void print_elf_header(Elf32_Ehdr *elf_raw);
void print_elf_program_header(Elf32_Phdr *ph);
void print_elf(Elf32_Ehdr *b);
Thread *load_elf(Elf32_Ehdr *elf);
#endif