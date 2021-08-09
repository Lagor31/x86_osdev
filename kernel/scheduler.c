#include "scheduler.h"

Thread *do_schedule() {
  List *l;

  u32 proc_num = 0;
  u32 pAvg = 0;
  u32 pTot = 0;
  Thread *next = NULL;

  u32 min_runtime = 0;

    list_for_each(l, &running_queue) {
    Thread *p = (Thread *)list_entry(l, Thread, head);
    if (proc_num == 0) {
      min_runtime = p->runtime;
      next = p;
    } else if (p->runtime < min_runtime) {
      min_runtime = p->runtime;
      next = p;
    }
    pTot += p->nice;
    ++proc_num;
  }

  pAvg = pTot / (proc_num);

  int q = MAX_QUANTUM_MS / proc_num;
  int penalty = (((int)pAvg - (int)next->nice) * P_PENALTY);
  q += penalty;

  if (q <= 0)
    next->sched_count = millis_to_ticks(MIN_QUANTUM_MS);
  else
    next->sched_count = millis_to_ticks((u32)q);
  return next;
}

u32 wake_up_all() {
  List *l;
  int c = 0;
wake_up:
  if (list_length(&sleep_queue) > 0) {
    list_for_each(l, &sleep_queue) {
      Thread *p = list_entry(l, Thread, head);
      if (p->sleeping_lock != NULL && p->sleeping_lock->state == LOCK_FREE) {
        p->sleeping_lock = NULL;
        // p->sched_count = 0;
        wake_up_thread(p);
        c++;
        goto wake_up;

      } else if (p->sleep_timer != 0 && tick_count >= p->sleep_timer) {
        p->sleep_timer = 0;
        // p->sched_count = 0;
        wake_up_thread(p);
        c++;
        goto wake_up;
      }
    }
  }
  return c;
}
