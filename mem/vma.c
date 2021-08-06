#include "vma.h"
#include "../mem/mem.h"
#include "../drivers/screen.h"
#include "../kernel/kernel.h"
#include "../lib/functions.h"

VMRegion *kernel_vm;

void init_kernel_vma(u32 base_address) {
  kernel_vm = (VMRegion *)kernel_page_alloc(0);
  kernel_vm->start = KERNEL_VIRTUAL_ADDRESS_BASE;
  kernel_vm->end = 0xFFFFFFFF;
  LIST_INIT(&kernel_vm->head);
  UNUSED(base_address);
}

bool is_valid_va(u32 va) {
  UNUSED(va);
  /*  kprintf("Checking address %x in [%x - %x]\n", va, kernel_vm->start,
           kernel_vm->end); */
  return (va <= kernel_vm->end && va >= kernel_vm->start);
}