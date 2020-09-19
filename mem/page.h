#ifndef PAGING_H
#define PAGING_H

#include "../utils/list.h"

#define PAGE_SIZE 4096

typedef struct Page {
  // 8 Bits for various flags
  char buddy_info;
  // Usage count
  int count;
  // Free pages list for buddy allocator
  List list;
} Page;

#endif