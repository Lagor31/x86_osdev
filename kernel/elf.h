#ifndef ELF__H
#define ELF__H

#include "../cpu/types.h"
#include "../drivers/screen.h"
#include <elf.h>

void print_elf_header(Elf32_Ehdr *elf_raw);
void print_elf_program_header(Elf32_Phdr *ph);
#endif 