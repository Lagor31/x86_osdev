#include "vma.h"
#include "../mem/mem.h"
#include "../drivers/screen.h"
#include "../kernel/kernel.h"
#include "../lib/utils.h"
#include "../mem/mem.h"
#include "../mem/mem_desc.h"

// VMArea *kernel_vm_area;
MemDesc *kernel_mem;

void init_kernel_vma() {
  kernel_mem = (MemDesc *)kernel_page_alloc(0);
  kernel_mem->areas_count = 1;
  LIST_INIT(&kernel_mem->vm_areas);
  VMArea *kernel_vm_area =
      create_vmregion(KERNEL_VIRTUAL_ADDRESS_BASE, 0xFFFFFFFF,
                      PA(KERNEL_VIRTUAL_ADDRESS_BASE), 0);
  list_add_tail(&kernel_mem->vm_areas, &kernel_vm_area->head);
  kernel_mem->usage_counter = 1;
}

VMArea *create_vmregion(u32 base_address, u32 end_address, u32 phys_start,
                        u32 flags) {
  VMArea *out_region = kernel_page_alloc(0);
  out_region->start = base_address;
  out_region->end = end_address;
  out_region->phys_start = phys_start;
  out_region->size = end_address - base_address;
  out_region->flags = flags;
  LIST_INIT(&out_region->head);
  return out_region;
}

bool is_valid_va(u32 va, Thread *t) {
  if (t == NULL) return TRUE;
  /*  kprintf("Checking address %x in [%x - %x]\n", va, kernel_vm->start,
           kernel_vm->end); */
  VMArea *vma;
  List *l;
  list_for_each(l, &t->mem->vm_areas) {
    vma = list_entry(l, VMArea, head);
    if (va < vma->end && va >= vma->start) return TRUE;
  }
  return FALSE;
}