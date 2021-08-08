#include "../cpu/types.h"
#include "../drivers/keyboard.h"
#include "../drivers/screen.h"
#include "../lib/strings.h"
#include "../lock/lock.h"

#include "thread.h"

List kwork_queue;
Thread *kwork_thread;

void init_work_queue() { LIST_INIT(&kwork_queue); }

void work_queue_thread() {
  List *wq;
  Work *do_me;

  while (TRUE) {
    // We can't be interrupted when updating the work queue
    disable_int();
    // I should always be able to acquire this lock.
    // It serves only to communicate with the scheduler that we're still doing
    // some high priority work and we don't want to be scheduled away
    work_queue_lock->state = LOCK_LOCKED;
    u8 found = 0;
    list_for_each(wq, &kwork_queue) {
      do_me = list_entry(wq, Work, work_queue);
      list_remove(&do_me->work_queue);
      found = 1;
      break;
    }

    if (found == 1) {
      // kprintf("Doing work!\n", (char)do_me->c);
      if (stdin->lock->state == LOCK_LOCKED)
        list_add(&kwork_queue, &do_me->work_queue);
      else
        write_byte_stream(stdin, do_me->c);
    }

    unlock(work_queue_lock);
    // Free lock means there's bytes to be read
    enable_int();
  }
}