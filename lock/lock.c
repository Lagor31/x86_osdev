#include "lock.h"
#include "../drivers/keyboard.h"
#include "../mem/mem.h"
#include "../proc/proc.h"
#include "../utils/list.h"

List kernel_locks;
u32 locks_id = 0;

Lock *screen_lock;
Lock *mem_lock;

void init_kernel_locks() {
  LIST_INIT(&kernel_locks);
  screen_lock = make_lock();
  mem_lock = make_lock();
}

Lock *make_lock() {
  Lock *outSpin = (Lock *)normal_page_alloc(0);
  outSpin->id = locks_id++;
  outSpin->state = LOCK_FREE;
  list_add(&kernel_locks, &outSpin->head);
  return outSpin;
}

void lock_spin(Lock *l) { _spin_lock(&l->state); }

void lock_sleep(Lock *l) {
  while (_test_spin_lock(&l->state) == LOCK_LOCKED) {
    current_proc->sleeping_lock = l;
    sleep_process(current_proc);
    _switch_to_task((Proc *)do_schedule());
  }
  current_proc->sleeping_lock = NULL;
}

void free_spin(Lock *l) {
  // current_proc->sleeping_lock = NULL;
  _free_lock(&l->state);
}
