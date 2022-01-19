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
  // Init of the 3 slab lists for totally empty, totally free and partially used
  LIST_INIT(&kMemCache.free);
  LIST_INIT(&kMemCache.empty);
  LIST_INIT(&kMemCache.used);
  // Since we don't have locks yet, we cannot sleep on memalloc
  kMemCache.empty_lock = make_lock_nosleep();
  kMemCache.used_lock = make_lock_nosleep();
  kMemCache.free_lock = make_lock_nosleep();

  // Creating some potentially useful slabs at kernel init
  createSlab(4, TRUE);
  createSlab(8, TRUE);
  createSlab(16, TRUE);
  createSlab(32, TRUE);
  createSlab(64, TRUE);
  createSlab(128, TRUE);

  // Creating some more slabs of the size of frequently used sizes in the kernel
  // Setting them as pinned so that the 'cleaning' thread won't free them
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
  // Looking for a slab starting at the partially used list
  List* p;
  list_for_each(p, &kMemCache.used) {
    Slab* s = list_entry(p, Slab, head);
    if (size == s->size) return s;
  }
  // Searching in the empty one if cannot find one above
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

  // Performs some checks about the address to be freed
  if (!is_kernel_addr((u32)buf)) {
    ret_val = FALSE;
    goto done;
  }
  Slab* s = buf->slab;
  // Making sure that the address is a Buf belonging to a cache
  if (!is_kernel_addr((u32)s) || s->fingerprint != SLAB_FINGERPRINT ||
      s->alloc <= 0) {
    /*  setBackgroundColor(RED);
     setTextColor(WHITE);
     kprintf("Error freeing cached obj 0x%x\n", b);
     resetScreenColors(); */
    ret_val = FALSE;
    goto done;
  }

  // Updating stats and first_free pointer for future allocations
  s->alloc--;
  buf->next_free = s->first_free;
  s->first_free = buf;
  s->last_used = tick_count;
  list_remove(&s->head);
  //  unlock(s->slock);
  // If slab's free, we move it to the free list
  if (s->alloc == 0) {
    // get_lock(kMemCache.free_lock);
    list_add_tail(&kMemCache.free, &s->head);
    // unlock(kMemCache.free_lock);
  } else {
    // Or we move it to the partially used one
    //  get_lock(kMemCache.used_lock);
    list_add_head(&kMemCache.used, &s->head);
    // unlock(kMemCache.used_lock);
  }
  ret_val = TRUE;
done:
  unlock(slab_lock);
  return ret_val;
}
// The slab alloc function as gets called by kmalloc in case of sizes << 4096
void* salloc(u32 size) {
  List* p;
  void* outP = NULL;
  get_lock(slab_lock);
  // get_lock(kMemCache.used_lock);

  // Always look first into the partially used one
  list_for_each(p, &kMemCache.used) {
    Slab* s = list_entry(p, Slab, head);
    // Looking for a slab of the right size
    if (size == s->size) {
      // Return the first free buffer
      Buf* out = s->first_free;
      if (out == NULL) return NULL;
      // get_lock(s->slock);
      // Update some stats about the slab and set the slabs first free buffer to
      // the Buf next_free pointer
      s->alloc++;
      s->first_free = out->next_free;
      out->next_free = NULL;
      // Moving the slab to the empty list if we just allocated the last block
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

  // Pretty much the same as abovoe
  // We look into the totally free one and move it to the partially used list
  // after our first allocation
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
// Creating a slab for chunks of given size
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

  // Calculating the chain of free Buf pointers
  // They point to the actual data
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