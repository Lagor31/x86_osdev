#ifndef SLAB_H
#define SLAB_H
#include "../lib/list.h"
#include "../lock/lock.h"

#define MAX_SLAB_SIZE 512
#define SLAB_FINGERPRINT 0xDEADBEEF

typedef struct Buf Buf;

// Must always be contained in a order 0 buddy block
typedef struct slab {
  u32 size;
  bool pinned;
  u32 last_used;
  u32 fingerprint;
  List head;
  u32 tot;
  u32 alloc;
  Buf *first_free;
} Slab ;

typedef struct Buf {
  Slab *slab;
  Buf *next_free;
} Buf;

typedef struct mem_cache {
  List free;
  Lock *free_lock;
  List used;
  Lock *used_lock;
  List empty;
  Lock *empty_lock;
} MemCache;

extern MemCache kMemCache;
Slab *createSlab(u32 size, bool nosleep);
Slab *find_slab(u32 size);

void init_slab_cache();
void *salloc(u32 size);
bool sfree(void *p);

#endif