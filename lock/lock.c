#include "lock.h"
#include "../drivers/keyboard.h"
#include "../mem/mem.h"
#include "../proc/thread.h"
#include "../lib/list.h"
#include "../kernel/scheduler.h"

List kernel_locks;
u32 locks_id = 0;

Lock *screen_lock;
Lock *mem_lock;
Lock *sched_lock;
Lock *work_queue_lock;

void disable_int() { __asm__ __volatile__("cli"); }

void enable_int() { __asm__ __volatile__("sti"); }

void init_kernel_locks() {
  LIST_INIT(&kernel_locks);
  screen_lock = make_lock();
  mem_lock = make_lock();
  sched_lock = make_lock();
  work_queue_lock = make_lock();
  work_queue_lock->state = LOCK_LOCKED;
  work_queue_lock->owner = NULL;
}

Lock *make_lock() {
  Lock *outSpin = (Lock *)normal_page_alloc(0);
  outSpin->id = locks_id++;
  outSpin->state = LOCK_FREE;
  list_add(&kernel_locks, &outSpin->head);
  outSpin->wait_q = create_wait_queue();
  return outSpin;
}

void spin_lock(Lock *l) { _spin_lock(&l->state); }

u32 test_lock(Lock *l) { return _test_spin_lock(&l->state); }

void get_lock(Lock *l) {
  while (test_lock(l) == LOCK_LOCKED) {
    // current_thread->sleeping_lock = l;
    list_add(&l->wait_q->threads_waiting, &current_thread->waitq);
    // if (l->owner != NULL) kprintf("Lock kept by: %s!", l->owner->command);
    sleep_thread(current_thread);
    reschedule();
  }
  // current_thread->sleeping_lock = NULL;
  l->owner = current_thread;
}

void unlock(Lock *l) {
  // current_proc->sleeping_lock = NULL;
  l->owner = NULL;
  _free_lock(&l->state);

  // wake_up_all();
  List *tList;
wake_up_sleeping_threads:
  list_for_each(tList, &l->wait_q->threads_waiting) {
    Thread *waiting_thread = list_entry(tList, Thread, waitq);
    list_remove(&waiting_thread->waitq);
    wake_up_thread(waiting_thread);
    goto wake_up_sleeping_threads;
  }
  LIST_INIT(&l->wait_q->threads_waiting);
}
