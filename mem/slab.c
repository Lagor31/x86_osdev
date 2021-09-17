#include "slab.h"
#include "buddy.h"
#include "mem.h"
#include "../drivers/screen.h"
#include "../proc/thread.h"

MemCache kMemCache;

void init_slab_cache() {
  // Since we don't have locks yet, we cannot sleep on memalloc
  LIST_INIT(&kMemCache.free);
  LIST_INIT(&kMemCache.empty);
  LIST_INIT(&kMemCache.used);
  createSlab(4, TRUE);
  createSlab(8, TRUE);
  createSlab(16, TRUE);
  createSlab(32, TRUE);
  createSlab(64, TRUE);
  createSlab(128, TRUE);
  createSlab(sizeof(Thread), TRUE);
  createSlab(sizeof(Lock), TRUE);
  createSlab(sizeof(WaitQ), TRUE);
  createSlab(256, TRUE);
  createSlab(512, TRUE);
}

Slab* find_slab(u32 size) {
  List* p;
  list_for_each(p, &kMemCache.used) {
    Slab* s = list_entry(p, Slab, head);
    if (size == s->size) return s;
  }

  list_for_each(p, &kMemCache.free) {
    Slab* s = list_entry(p, Slab, head);
    // kprintf("- Cache: %d\n", s->size);
    if (size == s->size) return s;
  }

  return NULL;
}

void sfree(void* b) {
  Buf* buf = (Buf*)((u32)b - sizeof(Buf) + sizeof(void*));
  Slab* s = buf->slab;
  if (s->fingerprint != SLAB_FINGERPRINT || s->alloc <= 0) {
    setBackgroundColor(RED);
    setTextColor(WHITE);
    kprintf("Error freeing cached obj 0x%x\n", b);
    resetScreenColors();
    return;
  }

  s->alloc--;
  list_add_head(&s->free_blocks, &buf->q);
  if (s->alloc == 0) {
    list_remove(&s->head);
    list_add_tail(&kMemCache.free, &s->head);
  }
}

void* salloc(u32 size) {
  List* p;
  list_for_each(p, &kMemCache.used) {
    Slab* s = list_entry(p, Slab, head);
    if (size == s->size) {
      s->alloc++;
      List* l;
      list_for_each(l, &s->free_blocks) {
        Buf* b = list_entry(l, Buf, q);
        list_remove(&b->q);
        if (s->alloc >= s->tot) {
          list_remove(&s->head);
          list_add_tail(&kMemCache.empty, &s->head);
        }
        return &b->buf;
      }
    }
  }

  list_for_each(p, &kMemCache.free) {
    Slab* s = list_entry(p, Slab, head);
    // kprintf("- Cache: %d\n", s->size);
    if (size == s->size) {
      // kprintf("Found!- Cache: %d\n", s->size);
      s->alloc++;
      List* l;
      list_for_each(l, &s->free_blocks) {
        Buf* b = list_entry(l, Buf, q);
        list_remove(&b->q);
        list_remove(&s->head);
        list_add_tail(&kMemCache.used, &s->head);
        return &b->buf;
      }
    }
  }
  return NULL;
}

Slab* createSlab(u32 size, bool no_sleep) {
  Slab* slab;
  if (no_sleep)
    slab = (Slab*)kalloc_nosleep(0);
  else
    slab = (Slab*)kalloc_page(0);
  slab->size = size;
  slab->alloc = 0;
  slab->tot = (PAGE_SIZE - sizeof(Slab)) / (size + sizeof(Buf));
  slab->fingerprint = SLAB_FINGERPRINT;

  LIST_INIT(&slab->free_blocks);

  for (size_t i = 0; i < slab->tot; i++) {
    Buf* b = (Buf*)((u32)(slab + sizeof(Slab)) + i * (sizeof(Buf) + size));
    b->slab = slab;
    LIST_INIT(&b->q);
    list_add_tail(&slab->free_blocks, &b->q);
    // b->buf = &b->buf;
  }

  list_add_tail(&kMemCache.free, &slab->head);

  return slab;
}