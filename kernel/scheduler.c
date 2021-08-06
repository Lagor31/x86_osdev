#include "scheduler.h"


Thread *do_schedule() {
  List *l;

  u32 i = 0;
  u32 pAvg = 0;
  u32 pTot = 0;
  Thread *next = NULL;

  if (list_length(&kwork_queue) > 0) return kwork_thread;

  u32 proc_num = list_length(&running_queue);
  if (proc_num > 1) {
    list_for_each(l, &running_queue) {
      Thread *p = (Thread *)list_entry(l, Thread, head);
      if (i++ == 0 && p->sched_count > 0 && p->pid != IDLE_PID) return p;
      pTot += p->nice;
    }

    pAvg = pTot / (proc_num - 1);
    i = 0;

    list_for_each(l, &running_queue) {
      Thread *p = (Thread *)list_entry(l, Thread, head);
      if (i++ == 0) {
        next = p;
        break;
      }
    }
    list_remove(&next->head);
    list_add(&running_queue, &next->head);

    int q = MAX_QUANTUM_MS / proc_num;
    int penalty = (((int)pAvg - (int)next->nice) * P_PENALTY);
    q += penalty;

    if (q <= 0)
      next->sched_count = millisToTicks(MIN_QUANTUM_MS);
    else
      next->sched_count = millisToTicks((u32)q);

    list_for_each(l, &running_queue) {
      Thread *p = (Thread *)list_entry(l, Thread, head);
      if (i++ == 0) {
        next = p;
        break;
      }
    }
    return next;
  }

  idle_thread->sched_count = millisToTicks(MIN_QUANTUM_MS);
  return idle_thread;
}


void wake_up_all() {
  List *l;
wake_up:
  if (list_length(&sleep_queue) > 0) {
    list_for_each(l, &sleep_queue) {
      Thread *p = list_entry(l, Thread, head);
      if (p->sleeping_lock != NULL && p->sleeping_lock->state == LOCK_FREE) {
        p->sleeping_lock = NULL;
        wake_up_thread(p);
        goto wake_up;

      } else if (p->sleep_timer != 0 && tick_count >= p->sleep_timer) {
        p->sleep_timer = 0;
        wake_up_thread(p);
        goto wake_up;
      }
    }
  }
}
