#ifndef PAGING_H
#define PAGING_H

#include "../utils/list.h"

#define PAGE_SIZE 4096

typedef struct Page {
  // 8 Bits for various flags
  char flags;
  // Usage count
  int count;
} Page;

#endif