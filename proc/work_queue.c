#include "../cpu/types.h"
#include "../drivers/keyboard.h"
#include "../drivers/screen.h"
#include "../libc/strings.h"
#include "../lock/lock.h"

#include "proc.h"

List kwork_queue;
Proc *kwork_thread;

void init_work_queue() {
  LIST_INIT(&kwork_queue);
  kwork_thread = create_kernel_proc(work_queue_thread, NULL, "kworker");
}

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

    if (list_length(&kwork_queue) > 0) {

      list_for_each(wq, &kwork_queue) {
        do_me = list_entry(wq, Work, work_queue);
        list_remove(&do_me->work_queue);
        enable_int();
        break;
      }
      // kprintf("Doing work!\n", (char)do_me->c);
      append(stdin.buffer, do_me->c);
      stdin.available++;

    } else {
      unlock(work_queue_lock);
      // Free lock means there's bytes to be read
      unlock(stdin.read_lock);
      enable_int();
    }
  }
}