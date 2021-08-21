#include "elf.h"

/*

Magic:   7f 45 4c 46 01 01 01 00 00 00 00 00 00 00 00 00
  Class:                             ELF32
  Data:                              2's complement, little endian
  Version:                           1 (current)
  OS/ABI:                            UNIX - System V
  ABI Version:                       0
  Type:                              REL (Relocatable file)
  Machine:                           Intel 80386
  Version:                           0x1
  Entry point address:               0x0
  Start of program headers:          0 (bytes into file)
  Start of section headers:          64 (bytes into file)
  Flags:                             0x0
  Size of this header:               52 (bytes)
  Size of program headers:           0 (bytes)
  Number of program headers:         0
  Size of section headers:           40 (bytes)
  Number of section headers:         6
  Section header string table index: 3


*/

void print_elf_program_header(Elf32_Phdr *ph) {
  kprintf("Off: 0x%x VAddr: 0x%x Type: 0x%x Size: 0x%x Flags: 0x%x Align: %d\n",
          ph->p_offset, ph->p_vaddr, ph->p_type, ph->p_filesz, ph->p_flags,
          ph->p_align);
}

void print_elf_header(Elf32_Ehdr *h) {
  kprintf(
      "Version: %d\nMachine: %d\nType: %d\nEntry: 0x%x\n"
      "PHOff: 0x%x\nEHSize: %d\nPHNum %d\nPHESize: %d\n",
      h->e_version, h->e_machine, h->e_type, h->e_entry, h->e_phoff,
      h->e_ehsize, h->e_phnum, h->e_phentsize);
}
