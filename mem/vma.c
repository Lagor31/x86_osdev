#include "vma.h"
#include "../mem/mem.h"
#include "../drivers/screen.h"

VMRegion *kernel_vm;

void init_kernel_vma(u32 base_address) {
  kernel_vm = (VMRegion *)kernel_page_alloc(0);
  kernel_vm->start = base_address;
  kernel_vm->end = 0xFFFFFFFF;
  LIST_INIT(&kernel_vm->head);
}

bool is_valid_va(u32 va) {
  kprintf("Checking address %x in [%x - %x]\n", va, kernel_vm->start,
          kernel_vm->end);
  return (va <= kernel_vm->end && va >= kernel_vm->start);
}