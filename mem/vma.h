#ifndef VMA_H
#define VMA_H
#include "../lib/list.h"
#include "mem_desc.h"

#define VMA_REG 0
#define VMA_STACK 1

typedef struct virtual_memory_region {
  byte type;
  u32 start;
  u32 end;
  List head;
  u32 phys_start;
  u32 size;
  u32 flags;
} VMArea;

typedef struct Thread Thread;
// extern VMArea *kernel_vm_area;
extern MemDesc *kernel_mem;
void init_kernel_vma();
bool is_valid_va(u32 va, Thread *t);
VMArea *create_vmregion(u32 base_address, u32 end_address, u32 phys_start,
                        u32 flags, u32 type);
#endif