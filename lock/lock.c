#include "lock.h"
#include "../drivers/keyboard.h"
#include "../mem/mem.h"
#include "../proc/thread.h"
#include "../lib/list.h"

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
}

Lock *make_lock() {
  Lock *outSpin = (Lock *)normal_page_alloc(0);
  outSpin->id = locks_id++;
  outSpin->state = LOCK_FREE;
  list_add(&kernel_locks, &outSpin->head);
  return outSpin;
}

void spin_lock(Lock *l) { _spin_lock(&l->state); }

u32 test_lock(Lock *l) { return _test_spin_lock(&l->state); }

void get_lock(Lock *l) {
  while (_test_spin_lock(&l->state) == LOCK_LOCKED) {
    current_thread->sleeping_lock = l;
    sleep_thread(current_thread);
    _switch_to_thread((Thread *)do_schedule());
  }
  current_thread->sleeping_lock = NULL;
}

void unlock(Lock *l) {
  // current_proc->sleeping_lock = NULL;
  _free_lock(&l->state);
}
