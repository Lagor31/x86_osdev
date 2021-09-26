#include "lock.h"
#include "../drivers/keyboard.h"
#include "../mem/mem.h"
#include "../proc/thread.h"
#include "../lib/list.h"
#include "../kernel/scheduler.h"

List kernel_locks;
u32 locks_id = 0;

Lock *screen_lock;
Lock *kmem_lock;
Lock *nmem_lock;
Lock *slab_lock;

Lock *work_queue_lock;

bool disable_int() {
  bool i = ints_enabled();
  __asm__ __volatile__("cli");
  return i;
}

void enable_int(bool prev_state) {
  if (prev_state == TRUE) __asm__ __volatile__("sti");
}

void init_kernel_locks() {
  LIST_INIT(&kernel_locks);
  /*
    We would sleep on the mem alloc for this lock
  */
  kmem_lock = make_lock_nosleep();
  nmem_lock = make_lock_nosleep();
  slab_lock = make_lock_nosleep();
  screen_lock = make_lock_nosleep();
  work_queue_lock = make_lock_nosleep();
  work_queue_lock->state = LOCK_LOCKED;
  work_queue_lock->owner = NULL;
}

Lock *make_lock() {
  Lock *outSpin = (Lock *)kmalloc(sizeof(Lock));
  outSpin->id = locks_id++;
  outSpin->state = LOCK_FREE;
  bool pi = disable_int();
  list_add_head(&kernel_locks, &outSpin->head);
  enable_int(pi);
  outSpin->wait_q = create_wait_queue();
  return outSpin;
}

void destroy_lock(Lock *l) {
  list_remove(&l->head);
  destroy_wait_queue(l->wait_q);
  kfree(l);
}

Lock *make_lock_nosleep() {
  Lock *outSpin = (Lock *)kalloc_nosleep(0);
  outSpin->id = locks_id++;
  outSpin->state = LOCK_FREE;
  list_add_head(&kernel_locks, &outSpin->head);
  outSpin->wait_q = create_wait_queue_nosleep();
  return outSpin;
}

void spin_lock(Lock *l) { _spin_lock(&l->state); }

u32 test_lock(Lock *l) { return _test_spin_lock(&l->state); }

void get_lock(Lock *l) {
  while (test_lock(l) == LOCK_LOCKED) {
    // current_thread->sleeping_lock = l;
    if (current_thread->wait_flags == 0)
      list_add_head(&l->wait_q->threads_waiting, &current_thread->waitq);
    else
      list_add_tail(&l->wait_q->threads_waiting, &current_thread->waitq);
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
    if (waiting_thread->wait_flags == 1) break;
    goto wake_up_sleeping_threads;
  }
  // LIST_INIT(&l->wait_q->threads_waiting);
}
