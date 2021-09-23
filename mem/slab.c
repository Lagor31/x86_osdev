#include "slab.h"
#include "buddy.h"
#include "mem.h"
#include "../drivers/screen.h"
#include "../proc/thread.h"
#include "../lock/lock.h"
#include "../kernel/scheduler.h"
#include "../lock/lock.h"

MemCache kMemCache;

void init_slab_cache() {
  // Since we don't have locks yet, we cannot sleep on memalloc
  LIST_INIT(&kMemCache.free);
  LIST_INIT(&kMemCache.empty);
  LIST_INIT(&kMemCache.used);

  kMemCache.empty_lock = make_lock_nosleep();
  kMemCache.used_lock = make_lock_nosleep();
  kMemCache.free_lock = make_lock_nosleep();

  createSlab(4, TRUE);
  createSlab(8, TRUE);
  createSlab(16, TRUE);
  createSlab(32, TRUE);
  createSlab(64, TRUE);
  createSlab(128, TRUE);

  Slab* s = createSlab(sizeof(Thread), TRUE);
  s->pinned = TRUE;
  s = createSlab(sizeof(Lock), TRUE);
  s->pinned = TRUE;
  s = createSlab(sizeof(WaitQ), TRUE);
  s->pinned = TRUE;
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

bool sfree(void* b) {
  get_lock(slab_lock);
  bool ret_val = FALSE;
  Buf* buf = (Buf*)((u32)b - sizeof(Buf));
  if (!is_kernel_addr((u32)buf)) {
    ret_val = FALSE;
    goto done;
  }
  Slab* s = buf->slab;

  if (!is_kernel_addr((u32)s) || s->fingerprint != SLAB_FINGERPRINT ||
      s->alloc <= 0) {
    /*  setBackgroundColor(RED);
     setTextColor(WHITE);
     kprintf("Error freeing cached obj 0x%x\n", b);
     resetScreenColors(); */
    ret_val = FALSE;
    goto done;
  }
  //

  s->alloc--;
  buf->next_free = s->first_free;
  s->first_free = buf;
  s->last_used = tick_count;
  list_remove(&s->head);
  //  unlock(s->slock);

  if (s->alloc == 0) {
    // get_lock(kMemCache.free_lock);
    list_add_tail(&kMemCache.free, &s->head);
    // unlock(kMemCache.free_lock);
  } else {
    //  get_lock(kMemCache.used_lock);
    list_add_tail(&kMemCache.used, &s->head);
    // unlock(kMemCache.used_lock);
  }
  ret_val = TRUE;
done:
  unlock(slab_lock);
  return ret_val;
}

void* salloc(u32 size) {
  List* p;
  void* outP = NULL;
  get_lock(slab_lock);
  // get_lock(kMemCache.used_lock);
  list_for_each(p, &kMemCache.used) {
    Slab* s = list_entry(p, Slab, head);
    if (size == s->size) {
      Buf* out = s->first_free;
      if (out == NULL) return NULL;
      // get_lock(s->slock);
      s->alloc++;
      s->first_free = out->next_free;
      out->next_free = NULL;
      if (s->alloc >= s->tot) {
        s->alloc = s->tot;
        list_remove(&s->head);
        // I dont want to sleep holding the used lock
        // get_lock(kMemCache.empty_lock);
        list_add_tail(&kMemCache.empty, &s->head);
        // unlock(kMemCache.empty_lock);
      }
      // unlock(s->slock);
      // unlock(kMemCache.used_lock);

      outP = (void*)((u32)out + sizeof(Buf));
      goto done;
    }
  }
  // unlock(kMemCache.used_lock);

  // get_lock(kMemCache.free_lock);
  list_for_each(p, &kMemCache.free) {
    Slab* s = list_entry(p, Slab, head);
    if (size == s->size) {
      Buf* out = s->first_free;
      if (out == NULL) return NULL;
      // get_lock(s->slock);

      s->alloc++;
      s->first_free = out->next_free;
      out->next_free = NULL;

      list_remove(&s->head);
      // unlock(s->slock);

      //  unlock(kMemCache.free_lock);
      //  get_lock(kMemCache.used_lock);
      list_add_tail(&kMemCache.used, &s->head);
      //  unlock(kMemCache.used_lock);
      outP = (void*)((u32)out + sizeof(Buf));
      goto done;
    }
  }
//  unlock(kMemCache.free_lock);
done:
  unlock(slab_lock);
  return outP;
}

Slab* createSlab(u32 size, bool no_sleep) {
  Slab* slab;
  if (no_sleep)
    slab = (Slab*)kalloc_nosleep(0);
  else
    slab = (Slab*)kalloc_page(0);
  slab->size = size;
  slab->alloc = 0;
  slab->tot = (PAGE_SIZE - sizeof(Slab)) / (size + sizeof(Buf)) - 1;
  slab->fingerprint = SLAB_FINGERPRINT;
  slab->pinned = FALSE;
  slab->last_used = 0;
  // slab->slock = make_lock_nosleep();
  // TODO: Tidy up math

  u32 b = (u32)slab;
  u32 offset = sizeof(Slab);
  b += offset;
  Buf* fb = (Buf*)b;
  slab->first_free = fb;
  for (size_t i = 0; i <= slab->tot; i++) {
    fb->slab = slab;
    Buf* next = (Buf*)((u32)fb + sizeof(Buf) + size);
    if (i < slab->tot)
      fb->next_free = next;
    else
      fb->next_free = NULL;
    fb = next;
  }
  LIST_INIT(&slab->head);
  get_lock(slab_lock);
  list_add_tail(&kMemCache.free, &slab->head);
  unlock(slab_lock);
  return slab;
}