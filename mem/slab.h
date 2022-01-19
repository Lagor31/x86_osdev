#ifndef SLAB_H
#define SLAB_H
#include "../lib/list.h"
#include "../lock/lock.h"

#define MAX_SLAB_SIZE 512
#define SLAB_FINGERPRINT 0xDEADBEEF

typedef struct Buf Buf;

/*
The paper that first described slab caches and from which i drew inspiration to
write my own The Slab Allocator: An Object-Caching Kernel Memory Allocator
Jeff Bonwick
Sun Microsystems
https://people.eecs.berkeley.edu/~kubitron/courses/cs194-24-S13/hand-outs/bonwick_slab.pdf
*/

// Must always be contained in a order 0 buddy block
// This serves as a header, the rest of the bytes in a 4096 bytes page will
// contain the actual data
typedef struct slab {
  u32 size;         // Size of the returned chunks
  bool pinned;      // Won't get freed even if it's totally empty
  u32 last_used;    // Tick counts of when it was last used
  u32 fingerprint;  // A specific fingeprint to determine if a given pointer
                    // belongs to an actual slab
  List head;        // Will be added to a free/used/empty list
  u32 tot;          // How many block of the given size it holds
  u32 alloc;        // How  many blocks have been allocated
  Buf *first_free;  // Pointer to the first free buffer, it will be the pointer
                    // returned by kamlloc
} Slab;

// One of the slabs' buffers with a pointer to the enclosing slab and a pointer
// to the next free buffer
typedef struct Buf {
  Slab *slab;
  Buf *next_free;
} Buf;

typedef struct mem_cache {
  List free;  // 0 Allocations done
  Lock *free_lock;
  List used;  // Some but not all of the allocations are done
  Lock *used_lock;
  List empty;  // Cannot allocate any more blocks from this cache
  Lock *empty_lock;
} MemCache;

extern MemCache kMemCache;
Slab *createSlab(u32 size, bool nosleep);
Slab *find_slab(u32 size);

void init_slab_cache();
void *salloc(u32 size);
bool sfree(void *p);

#endif