#include "../cpu/types.h"
#include "../drivers/keyboard.h"
#include "../drivers/screen.h"
#include "../lib/strings.h"
#include "../lock/lock.h"
#include "../kernel/files.h"

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

    if (list_length(&kwork_queue) > 0) {
      list_for_each(wq, &kwork_queue) {
        do_me = list_entry(wq, Work, work_queue);
        list_remove(&do_me->work_queue);
        break;
      }

      enable_int();

      // kprintf("Doing work!\n", (char)do_me->c);
      /*  get_lock(stdin_t->lock);
       append(stdin_t->buffer, do_me->c);
       stdin.available++;
       unlock(stdin.read_lock); */

      write_byte_stream(stdin_t, do_me->c);

    } else {
      // Free lock means there's bytes to be read
      // unlock(stdin.read_lock);
      enable_int();

      work_queue_lock->state = LOCK_LOCKED;
      sleep_on_lock(current_thread, work_queue_lock);
      yield();
    }
  }
}