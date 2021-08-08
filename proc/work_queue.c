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

    // work_queue_lock->state = LOCK_LOCKED;
    u8 found = 0;
    list_for_each(wq, &kwork_queue) {
      do_me = list_entry(wq, Work, work_queue);
      list_remove(&do_me->work_queue);
      found = 1;
      break;
    }
    enable_int();

    if (found == 1) {
      // kprintf("Doing work!\n", (char)do_me->c);
      if (write_byte_stream(stdin, do_me->c) < 0) {
        // TODO: not so safe!
        disable_int();
        list_add(&kwork_queue, &do_me->work_queue);
        enable_int();
      }
    } else {
      current_thread->sleeping_lock = work_queue_lock;
      sleep_thread(current_thread);
      Thread *n = do_schedule();
      wake_up_thread(n);
      enable_int();
      _switch_to_thread(n);
    }

    // Free lock means there's bytes to be read
  }
}
