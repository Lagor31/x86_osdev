#include "elf.h"

#include "../proc/thread.h"
#include "../mem/paging.h"

Thread *load_elf(Elf32_Ehdr *elf) {
  MemDesc *thread_mem = normal_page_alloc(0);
  LIST_INIT(&thread_mem->vm_areas);
  thread_mem->page_directory = (u32)kernel_page_alloc(0);
  init_user_paging((u32 *)thread_mem->page_directory);
  int i = 0;
  Elf32_Phdr *ph = (Elf32_Phdr *)((u32)elf + (u32)elf->e_phoff);

  for (i = 0; i < elf->e_phnum; ++i) {
    VMArea *vm;
    u32 phys_addr;
    if (ph[i].p_type == 1) {
      if (ph[i].p_filesz == 0) {
        // Assuming it's a .bss 0-initialized section
        byte *bss = (byte *)normal_page_alloc(0);
        memset(bss, 0, PAGE_SIZE);
        phys_addr = PA((u32)bss);
      } else
        phys_addr = PA((u32)elf + ph[i].p_offset);

      vm = create_vmregion(ph[i].p_vaddr, ph[i].p_vaddr + ph[i].p_memsz,
                           phys_addr, ph[i].p_flags);
      list_add_tail(&thread_mem->vm_areas, &vm->head);
    }
  }

  Thread *t =
      create_user_thread((void *)(elf->e_entry), thread_mem, NULL, "elf_user");
  t->nice = MIN_PRIORITY;
  VMArea *stack = create_vmregion(USER_STACK_TOP - PAGE_SIZE, USER_STACK_TOP,
                                  PA((u32)t->tcb.user_stack_bot), 0);
  list_add_tail(&thread_mem->vm_areas, &stack->head);

  print_mem_desc(thread_mem);
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
