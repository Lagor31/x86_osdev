#include "scheduler.h"
#include "timer.h"
Thread *pick_next_thread() {
  List *l;

  u32 proc_num = 0;
  u32 pAvg = 0;
  u32 pTot = 0;
  Thread *next = NULL;

  unsigned long long min_runtime = 0;
  // disable_int();
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

  // enable_int();
  pAvg = pTot / (proc_num);

  int q = MAX_QUANTUM_MS / proc_num;
  int penalty = (((int)pAvg - (int)next->nice) * P_PENALTY);
  q += penalty;

  if (q <= 0)
    next->timeslice = millis_to_ticks(MIN_QUANTUM_MS);
  else
    next->timeslice = millis_to_ticks((u32)q);
  return next;
}

/*
Only executed in interrupt context with no int enabled
*/
u32 wake_up_timers() {
  int c = 0;
  List *tlist;
do_timers:
  if (list_length(&kernel_timers) > 0) {
    list_for_each(tlist, &kernel_timers) {
      Timer *activeT = list_entry(tlist, Timer, q);
      if (tick_count >= activeT->expiration) {
        list_remove(&activeT->q);
        wake_up_thread(activeT->thread);
        kfree_normal(activeT);
        c++;
        goto do_timers;
      }
    }
  }
  return c;
}

void reschedule() {
  Thread *next = pick_next_thread();
  unsigned long long now = rdtscl();
  next->last_activation = now;
  // In CPU cycles
  if (current_thread != idle_thread)
    current_thread->runtime += (now - current_thread->last_activation);
  _switch_to_thread(next);
}
