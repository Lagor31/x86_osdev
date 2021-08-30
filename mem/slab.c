#include "slab.h"
#include "buddy.h"
#include "mem.h"
#include "../drivers/screen.h"

MemCache kMemCache;

void kMemCacheInit() {
  LIST_INIT(&kMemCache.free->head);
  list_add_head(&kMemCache.free->head, &createSlab(8)->head);
  list_add_head(&kMemCache.free->head, &createSlab(16)->head);
  list_add_head(&kMemCache.free->head, &createSlab(16)->head);
  list_add_head(&kMemCache.free->head, &createSlab(32)->head);
  list_add_head(&kMemCache.free->head, &createSlab(48)->head);
  list_add_head(&kMemCache.free->head, &createSlab(64)->head);
  kMemCache.empty = NULL;
  kMemCache.used = NULL;
  List *p;
  list_for_each(p, &kMemCache.free->head) {
    Slab *s = list_entry(p, Slab, head);
    kprintf("- Cache: %d\n", s->size);
  }
}

Slab* createSlab(u8 size) {
  Slab* slab = (Slab*)kalloc(0);
  slab->size = size;
  return slab;
}