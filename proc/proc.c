#include "proc.h"
#include "../cpu/timer.h"
#include "../mem/mem.h"
#include "../mem/vma.h"
#include "../cpu/types.h"
#include "../utils/utils.h"
#include "../drivers/screen.h"
#include "../libc/functions.h"

List sleep_queue;
List ready_queue;
List stopped_queue;

Proc *current_proc = NULL;
static u32 pid = IDLE_PID;

void stop_process(Proc *p) {
  setBackgroundColor(RED);
  setTextColor(WHITE);
  kprintf("Stopping proc with PID: %d\n", p->pid);
  list_remove(&p->head);
  list_add(&stopped_queue, &p->head);
  if (current_proc == p) {
    load_current_proc(NULL);
  }
  resetScreenColors();
}

void sleep_process(Proc *p) {
  setBackgroundColor(YELLOW);
  setTextColor(BLUE);
  kprintf("Sleeping proc with PID: %d\n", p->pid);
  list_remove(&p->head);
  list_add(&sleep_queue, &p->head);
  if (current_proc == p) {
    load_current_proc(NULL);
  }
  resetScreenColors();
}

void do_schedule() {
  List *l;

  if (current_proc != NULL) {
    setBackgroundColor(DARKGRAY);
    kprintf("Current proc PID: %d\n", current_proc->pid);
    bool stop = (rand() % 20) == 0;
    if (stop == TRUE && current_proc != NULL && current_proc->pid != 0) {
      stop_process(current_proc);
      resetScreenColors();
      return;
    }

    bool sleep = (rand() % 10) == 0;
    if (sleep == TRUE && current_proc != NULL && current_proc->pid != 0) {
      sleep_process(current_proc);
      resetScreenColors();
      return;
    }
  }

  if (list_length(&sleep_queue) > 0) {
    kprintf("SLEEP Q:\n");
    setBackgroundColor(BLUE);
    list_for_each(l, &sleep_queue) {
      Proc *p = list_entry(l, Proc, head);
      kprintf("PID: %d\n", p->pid);

      bool wakeup = (rand() % 2) == 0;
      if (wakeup == TRUE) {
        wake_up_process(p);
        return;
      }
    }
  }

  if (list_length(&ready_queue) > 0) {
    kprintf("READY Q:\n");
    setBackgroundColor(GREEN);
    setTextColor(BLACK);
    list_for_each(l, &ready_queue) {
      Proc *p = list_entry(l, Proc, head);
      kprintf("PID: %d\n", p->pid);
      if (current_proc != p && p->p <= current_proc->p) {
        kprintf("Scheduling new proc with PID:%d\n", p->pid);
        wake_up_process(p);
        load_current_proc(p);
        break;
      }
    }
  }
  resetScreenColors();

  if (list_length(&stopped_queue) > 0) {
    kprintf("STOPPED Q:\n");
    setBackgroundColor(RED);
    setTextColor(BLUE);
    list_for_each(l, &stopped_queue) {
      Proc *p = list_entry(l, Proc, head);
      kprintf("PID: %d\n", p->pid);
    }
  }
  resetScreenColors();
}

void load_current_proc(Proc *p) { current_proc = p; }
void wake_up_process(Proc *p) {
  resetScreenColors();
  setBackgroundColor(PURPLE);
  setTextColor(WHITE);
  kprintf("Waking proc with PID: %d\n", p->pid);
  list_remove(&p->head);
  list_add(&ready_queue, &p->head);
  resetScreenColors();
}

void printProc(Proc *p) {
  kprintf(
      "N: %s - PID: %d - P: %d - isKProc: %d - EIP: %x - ESP: %x - VMR: %x - "
      "PDIR: "
      "%x\n",
      p->name, p->pid, p->p, p->isKernelProc, p->regs.eip, p->regs.esp, p->Vm,
      p->page_dir);
}

void init_kernel_proc() {
  LIST_INIT(&sleep_queue);
  LIST_INIT(&ready_queue);
  LIST_INIT(&stopped_queue);
  current_proc = NULL;
  Proc *idle_proc = create_kernel_proc(idle, NULL, "kernel_idle_process");
  idle_proc->pid = IDLE_PID;
  idle_proc->p = MIN_PRIORITY;

  wake_up_process(idle_proc);
  load_current_proc(idle_proc);

  init_scheduler_timer();
}

int idle() {
  while (TRUE) {
    hlt();
  }
  return -1;
}

Proc *create_kernel_proc(int (*procfunc)(void *input), void *data,
                         const char *args, ...) {
  // TODO: cache! chache! cache!
  Proc *kernel_process = kernel_page_alloc(0);
  void *kernel_stack = kernel_page_alloc(0);
  const char *proc_name = (const char *)kernel_page_alloc(0);
  proc_name = args;
  kernel_process->isKernelProc = TRUE;
  kernel_process->name = proc_name;
  kernel_process->p = 0;
  kernel_process->pid = pid++;
  LIST_INIT(&kernel_process->head);
  kernel_process->page_dir = (u32 **)&kernel_page_directory;
  kernel_process->Vm = kernel_vm;
  kernel_process->regs.eip = (u32)procfunc;
  kernel_process->regs.esp = (u32)kernel_stack;

  UNUSED(data);
  return kernel_process;
}