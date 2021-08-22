#include "elf.h"

#include "../proc/thread.h"

Thread *load_elf(Elf32_Ehdr *elf) {
  Thread *t = create_user_thread((void *)(elf->e_entry - 0x08048000 + (u32)elf),
                                 NULL, "elf_usr");

  return t;
}

void print_elf(Elf32_Ehdr *b) {
  kprintf("%c%c%c\n", b->e_ident[1], b->e_ident[2], b->e_ident[3]);
  print_elf_header(b);
  Elf32_Phdr *ph = (Elf32_Phdr *)((u32)b + (u32)b->e_phoff);
  int i = 0;
  kprintf("Program Headers:\n");
  for (i = 0; i < b->e_phnum; ++i) {
    kprintf("[%d] ", i);
    print_elf_program_header(&ph[i]);
  }
}

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
