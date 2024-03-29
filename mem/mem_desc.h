#ifndef MEM_DESC_H
#define MEM_DESC_H

#include "../lib/list.h"
#include "../lock/lock.h"

typedef struct memory_descriptor_t {
  u32 areas_count;
  u32 usage_counter;
  Lock *mem_lock;
  List vm_areas;
  u32 code_start;
  u32 code_size;
  u32 data_start;
  u32 data_size;
  u32 heap_start;
  u32 heap_end;
  u32 stack_top;
  u32 page_directory;
} MemDesc;

#endif