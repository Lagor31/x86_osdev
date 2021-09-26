#include "signals.h"
#include "../drivers/screen.h"
#include "../proc/thread.h"

void init_signals(Signals *sigs) {
  sigs->active = ALL_SIGNALS;
  sigs->pending = 0;
}

u32 sys_kill(u32 pid, u32 signal) {
  // kprintf("Delivering signal %d to pid %d\n", signal, pid);
  Thread *t = get_thread(pid);
  if (t != NULL) {
    t->signals.pending |= (t->signals.active & signal);
    return signal;
  }
  return -1;
}

u32 child_awoken(u32 pid) {
  // kprintf("Delivering signal %d to pid %d\n", signal, pid);
  Thread *t = get_thread(pid);
  if (t != NULL) {
    t->signals.pending |= (t->signals.active & SIGCKO);
    u32 prev_state = t->state;
    u32 pi = disable_int();
    wake_up_thread(t);
    t->state = prev_state;
    enable_int(pi);
    return SIGCKO;
  }
  return -1;
}

const char *get_sig_name(u32 sig_num) {
  if (sig_num == SIGKILL) return "SIGKILL";
  if (sig_num == SIGCKO) return "SIGCKO";

  return "UNKSIG";
}

bool handle_signals(Thread *t) {
  if (t == NULL) return TRUE;
  Signals *sigs = &t->signals;
  if (sigs->pending == 0) return TRUE;
  i32 i = 1;
  bool ret = TRUE;
  for (i = 1; i <= SIG_NUM; i++) {
    if ((sigs->pending & sigs->active) & 1 << i) {
      kprintf("Received sig %s for pid %d\n", get_sig_name(1 << i), t->pid);
      sigs->pending &= ~(1 << i);
      // Kill thread
      if (1 << i == SIGKILL) {
        kill_process(t);
        t->exit_value = 0;
        ret = FALSE;
      } else if (1 << i == SIGCKO) {
        // kprintf("Child awoken!\n");
        List *l;
        bool all_ko = TRUE;
        bool wait4pid = FALSE;
        list_for_each(l, &t->children) {
          Thread *p1 = (Thread *)list_entry(l, Thread, siblings);
          if (p1->state != TASK_STOPPED && p1->state != TASK_ZOMBIE)
            all_ko = FALSE;
          if (t->wait4 == p1->pid &&
              (p1->state == TASK_STOPPED || p1->state == TASK_ZOMBIE)) {
            wait4pid = TRUE;
            reparent(t, &p1->children);
            // list_remove(&p1->children);
            // list_remove(&p1->siblings);
            // p1->state = TASK_ZOMBIE;
          }
        }

        if (t->wait4child && all_ko) {
          list_for_each(l, &t->children) {
            Thread *p1 = (Thread *)list_entry(l, Thread, siblings);
            reparent(t, &p1->children);
            // list_remove(&p1->children);
            // list_remove(&p1->siblings);
            // p1->state = TASK_ZOMBIE;
            //wake_up_thread(t);
          }
        }
        if (t->wait4child && !all_ko) {
      /*     list_for_each(l, &t->children) {
            Thread *p1 = (Thread *)list_entry(l, Thread, siblings);
            reparent(t, &p1->children); */
            // list_remove(&p1->children);
            // list_remove(&p1->siblings);
            // p1->state = TASK_ZOMBIE;
            sleep_thread(t);
          ret = FALSE;
        }
        // if (wait4pid) ret = TRUE;
        if (!t->wait4child && !wait4pid) {
          list_for_each(l, &t->children) {
            Thread *p1 = (Thread *)list_entry(l, Thread, siblings);
            if (p1->state == TASK_STOPPED) {
              reparent(t, &p1->children);
              // list_remove(&p1->children);
              // list_remove(&p1->siblings);
              p1->state = TASK_ZOMBIE;
            }
          }

          if (t->state == TASK_INTERRUPTIBLE) {
            sleep_thread(t);
            ret = FALSE;
          }
        }
      }
    }
  }
  return ret;
}