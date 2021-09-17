#ifndef SLAB_H
#define SLAB_H
#include "../lib/list.h"
typedef struct Buf Buf;

// Must always be contained in a order 0 buddy block
typedef struct slab {
  u16 size;
  List head;
  u32 tot;
  u32 alloc;
  List free_blocks;
} Slab;

typedef struct Buf {
  Slab *slab;
  List q;
  void *buf;
} Buf;

typedef struct mem_cache {
  List free;
  List used;
  List empty;
} MemCache;

extern MemCache kMemCache;
Slab *createSlab(u16 size);
void kMemCacheInit();
void *salloc(u32 size);
void sfree(void *p);

#endif