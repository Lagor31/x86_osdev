#ifndef VMA_H
#define VMA_H
#include "../utils/list.h"

typedef struct virtual_memory_region {
  u32 start;
  u32 end;
  List head;
} VMRegion;

extern VMRegion *kernel_vm;
void init_kernel_vma(u32 base_address);
bool is_valid_va(u32 va);

#endif