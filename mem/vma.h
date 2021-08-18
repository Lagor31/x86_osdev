#ifndef VMA_H
#define VMA_H
#include "../lib/list.h"

typedef struct virtual_memory_region {
  u32 start;
  u32 end;
  List head;
} VMRegion;

typedef struct Thread Thread;

extern VMRegion *kernel_vm;
void init_kernel_vma(u32 base_address);
bool is_valid_va(u32 va, Thread *t);

#endif