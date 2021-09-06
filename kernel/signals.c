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

const char *get_sig_name(u32 sig_num) {
  if (sig_num == SIGKILL) return "SIGKILL";
  return "UNKSIG";
}

bool handle_signals(Thread *t) {
  if (t == NULL) return TRUE;
  Signals *sigs = &t->signals;
  if (sigs->pending == 0) return TRUE;
  i32 i = 1;
  for (i = 1; i <= SIG_NUM; i++) {
    if ((sigs->pending & sigs->active) & 1 << i) {
      kprintf("Received sig %s pid %d\n", get_sig_name(1 << i), t->pid);
      sigs->pending &= ~(1 << i);
      // Kill thread
      if (1 << i == SIGKILL) {
        kill_process(t);
        return FALSE;
      }
    }
  }
  return TRUE;
}