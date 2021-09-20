#include "binaries.h"

void kswap() {
  while (TRUE) {
    //kprintf("Swap!\n");
    sleep_ms(5000);
    Thread *t;
    // get_lock(sched_lock);
    List *wq;
    Work *do_me;
    List *tem;
    list_for_each_safe(wq,tem, &kwork_queue) {
      do_me = list_entry(wq, Work, work_queue);
      list_remove(&do_me->work_queue);
      //kprintf("Cleanup after %d\n", do_me->t->pid);
      exterminate(t);
      kfree(do_me);
    }
  }
}
