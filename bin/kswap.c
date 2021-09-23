#include "binaries.h"
#include "../mem/slab.h"
void reap(Thread *p) {
  bool pi = disable_int();
  list_remove(&p->k_proc_list);
  list_remove(&p->head);
  enable_int(pi);
  if (p->ring0 == FALSE) {
    VMArea *vma;
    List *l;
    list_for_each(l, &p->mem->vm_areas) {
      vma = list_entry(l, VMArea, head);
      kfree(vma);
    }
    u32 c = 0;
    u32 *pd = (u32 *)VA(p->tcb.page_dir);

    for (size_t i = 0; i < PD_SIZE; i++) {
      if (isPresent(&pd[i]) && isUsermode(&pd[i])) {
        c++;
        ffree((void *)VA(pd[i] & 0xFFFFF000));
      }
    }
    c *= PAGE_SIZE / 1024;
    kfree_page((void *)p->mem->page_directory);
    kfree(p->mem);

    // kprintf("Freed proc(0x%x)\n", (u32)p);
  }

  kfree_page((void *)p->tcb.user_stack_bot);
  kfree_page((void *)p->tcb.kernel_stack_bot);
  kfree(p->command);
  kfree(p);
}

void exterminate() {
  List *sq;
  Thread *do_me;
  List *tem;
  // int i = 0;
  while (TRUE) {
    list_for_each_safe(sq, tem, &stopped_queue) {
      do_me = list_entry(sq, Thread, head);
      //++i;
      // kprintf("Cleanup after %d\n", do_me->t->pid);
      reap(do_me);
      bool pi = disable_int();
      list_remove(&do_me->head);
      enable_int(pi);
    }
    sleep_ms(5000);
  }
}

void kswap() {
  while (TRUE) {
    // kprintf("Swap!\n");
    sleep_ms(1000);
    Thread *t = create_kernel_thread(exterminate, NULL, "exterminator");
    t->nice = MIN_PRIORITY;
    wake_up_thread(t);
    List *s;
    Slab *do_me;
    List *tem;
    while (TRUE) {
      sleep_ms(10000);
      list_for_each_safe(s, tem, &kMemCache.free) {
        do_me = list_entry(s, Slab, head);
        if (do_me->pinned) continue;
        bool pi = disable_int();
        list_remove(&do_me->head);
        enable_int(pi);
        kfree_page(do_me);
      }
    }
  }
}
