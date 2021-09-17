#ifndef SLAB_H
#define SLAB_H
#include "../lib/list.h"

#define MAX_SLAB_SIZE 512
#define SLAB_FINGERPRINT 0xDEADBEEF

typedef struct Buf Buf;

// Must always be contained in a order 0 buddy block
typedef struct slab {
  u16 size;
  u32 fingerprint;
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
Slab *createSlab(u32 size);
Slab* find_slab(u32 size);

void kMemCacheInit();
void *salloc(u32 size);
void sfree(void *p);

#endif