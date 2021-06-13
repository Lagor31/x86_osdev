#ifndef SLAB_H
#define SLAB_H
#include "../utils/list.h"

// Must always be contained in a order 0 buddy block
typedef struct slab {
  u8 size;
  List head;
  u8 first_free;
  u8 *free_blocks;
} Slab;


typedef struct mem_cache {
  Slab *free;
  Slab *used;
  Slab *empty;
} MemCache;

extern MemCache kMemCache;
Slab* createSlab(u8 size); 
void kMemCacheInit();

#endif