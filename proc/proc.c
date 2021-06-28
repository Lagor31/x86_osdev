#include "proc.h"
#include "../cpu/timer.h"
#include "../cpu/types.h"
#include "../drivers/screen.h"
#include "../kernel/kernel.h"
#include "../libc/constants.h"
#include "../libc/functions.h"
#include "../libc/strings.h"
#include "../mem/mem.h"
#include "../mem/vma.h"
#include "../utils/utils.h"

List sleep_queue;
List running_queue;
List stopped_queue;

Proc *current_proc = NULL;
Proc *idle_proc;
static u32 pid = IDLE_PID;

void top() {
  List *l;
  Proc *p;

  while (TRUE) {

    asm volatile("cli");
    setCursorPos(1, 0);
    setBackgroundColor(LIGHTGREEN);
    setTextColor(BLACK);
    kprintf("[RUNNING]\n");
    u32 c = 0;
    list_for_each(l, &running_queue) {
      p = list_entry(l, Proc, head);
      kprintf("[%d] ", c++);
      printProcSimple(p);
    }
    resetScreenColors();

    c = 0;
    setBackgroundColor(LIGHTCYAN);
    setTextColor(BLACK);
    kprintf("[SLEEP]\n");
    list_for_each(l, &sleep_queue) {
      p = list_entry(l, Proc, head);
      kprintf("[%d] ", c++);
      printProcSimple(p);
    }
    resetScreenColors();

    c = 0;
    setBackgroundColor(WHITE);
    setTextColor(RED);
    kprintf("[STOPPED]\n");
    list_for_each(l, &stopped_queue) {
      p = list_entry(l, Proc, head);
      kprintf("[%d] ", c++);
      printProcSimple(p);
    }
    resetScreenColors();

    if (current_proc != NULL) {
      setBackgroundColor(GREEN_ON_BLACK);
      kprintf("[CURRENT]: \n");
      printProcSimple(current_proc);
      resetScreenColors();
    }
    asm volatile("sti");
    syncWait(100);
  }
}

void stop_process(Proc *p) {
   if(p->pid == IDLE_PID)
    return;
  // kprintf("Stopping process PID %d\n", p->pid);
  list_remove(&p->head);
  list_add(&stopped_queue, &p->head);
}

void sleep_process(Proc *p) {
  if(p->pid == IDLE_PID)
    return;
  // kprintf("Sleeping process PID %d\n", p->pid);
  list_remove(&p->head);
  list_add(&sleep_queue, &p->head);

  // do_schedule();
  //_switch_to_task(current_proc);
}

void u_simple_proc() {
  while (TRUE) {
    asm volatile("cli");
    // kprintf("PID: %d Name: %s\n", current_proc->pid, current_proc->name);
    sleep_process(current_proc);
    asm volatile("sti");

    /*
        Proc *me = current_proc;

        u32 r = rand();
        if (r % 10000000 == 0) {
          foo(r);
          foo(r);
          foo(r);
          foo(r);
          foo(r);
          foo(30);
          asm volatile("cli");
          kprintf("PID: %d Name: %s\n", current_proc->pid, current_proc->name);
          sleep_process(current_proc);
          asm volatile("sti");

          // do_schedule();
          //_switch_to_task(current_proc);
        } */
  }
}

Proc *do_schedule() {
  List *l;

  // List *l;
  /*   bool create = (rand() % 10000) == 0;
    if (create == TRUE) {
      Proc *n = create_user_proc(&u_simple_proc, NULL, "uproc-aaaa");
      n->p = rand() % 20;
      wake_up_process(n);
    } */

  /*   if (created++ < 1) {
      Proc *n = create_user_proc(&u_simple_proc, NULL, "uproc-aaaa");
      n->p = rand() % 20;
      wake_up_process(n);
    }
   */
  /* if (list_length(&stopped_queue) > 0) {
    list_for_each(l, &stopped_queue) {
      Proc *p = list_entry(l, Proc, head);
      bool kill = (rand() % 10) == 0;
      if (kill == TRUE) {
        kill_process(p);
      }
    }
  } */

  /* if (current_proc != NULL) {
    bool stop = (rand() % 10000) == 0;
    if (stop == TRUE && current_proc != NULL && current_proc->pid != IDLE_PID) {
      stop_process(current_proc);
      goto schedule_proc;
    }

    bool sleep = (rand() % 20000) == 0;
    if (sleep == TRUE && current_proc != NULL &&
        current_proc->pid != IDLE_PID) {
      sleep_process(current_proc);
      goto schedule_proc;
    }
  } */

schedule_proc:
  if (list_length(&sleep_queue) > 0) {
    list_for_each(l, &sleep_queue) {
      Proc *p = list_entry(l, Proc, head);
      //bool wakeup = (rand() % 2000) == 0;
      if (TRUE) {
        wake_up_process(p);
        break;
      }
    }
  }
/* 
   if (list_length(&stopped_queue) > 0) {
    list_for_each(l, &stopped_queue) {
      Proc *p = list_entry(l, Proc, head);
      bool wakeup = (rand() % 2000) == 0;
      if (wakeup == TRUE) {
        wake_up_process(p);
        break;
      }
    }
  }  */

  u32 i = 0;
  u32 pAvg = 0;
  u32 pTot = 0;
  Proc *next = NULL;
  u32 proc_num = list_length(&running_queue);
  if (proc_num > 1) {
    // u32 sched = rand() % size;
    list_for_each(l, &running_queue) {
      Proc *p = (Proc *)list_entry(l, Proc, head);
      if (i++ == 0) {
        next = p;
        continue;
      }

      pTot += p->p;
    }
  } else {
    idle_proc->sched_count = millisToTicks(MIN_QUANTUM_MS);
    return idle_proc;
  }
  pAvg = pTot / (proc_num - 1);

  if (next != NULL) {
    list_remove(&next->head);
    list_add(&running_queue, &next->head);

    int q = MAX_QUANTUM_MS / proc_num;
    int penalty = (((int)pAvg - (int)next->p) * P_PENALTY);
    q += penalty;

    if (q <= 0)
      next->sched_count = millisToTicks(MIN_QUANTUM_MS);
    else
      next->sched_count = millisToTicks((u32)q);

    return next;
  }

  idle_proc->sched_count = millisToTicks(MIN_QUANTUM_MS);
  return idle_proc;
}

void load_current_proc(Proc *p) { current_proc = p; }
void wake_up_process(Proc *p) {
  // kprintf("Waking up process PID %d\n", p->pid);
  list_remove(&p->head);
  list_add(&running_queue, &p->head);
}

void printProcSimple(Proc *p) {
  kprintf("%s - PID: %d - P: %d T: %dms\n", p->name, p->pid, p->p,
          ticksToMillis(p->running_ticks));
}
void printProc(Proc *p) {
  kprintf("%s - PID: %d - EIP: %x - ESP: %x - &N: 0x%x - Self: 0x%x\n", p->name,
          p->pid, p->regs.eip, p->regs.esp, p->name, p);
}

void init_kernel_proc() {
  LIST_INIT(&sleep_queue);
  LIST_INIT(&running_queue);
  LIST_INIT(&stopped_queue);
  idle_proc = create_kernel_proc(idle, NULL, "idle");
  idle_proc->pid = IDLE_PID;
  idle_proc->p = MIN_PRIORITY;
  wake_up_process(idle_proc);
  current_proc = idle_proc;
}

void idle() {
  while (TRUE) {
    hlt();
  }
}

Proc *create_user_proc(void (*procfunc)(), void *data, char *args, ...) {
  // TODO: cache! chache! cache!
  Proc *user_process = normal_page_alloc(0);

  user_process->isKernelProc = FALSE;

  user_process->p = 0;
  user_process->pid = pid++;
  LIST_INIT(&user_process->head);
  user_process->page_dir = (u32 **)&user_page_directory;
  user_process->Vm = kernel_vm;
  user_process->regs.eip = (u32)procfunc;

  void *user_stack = normal_page_alloc(0);
  user_process->regs.esp = (u32)user_stack + PAGE_SIZE;

  user_process->stack = user_stack;
  user_process->regs.ds = 0x23;
  user_process->regs.cs = 0x1B;
  user_process->regs.ss = 0x23;
  void *kernel_stack = normal_page_alloc(0);
  user_process->kernel_stack_top = kernel_stack;
  user_process->esp0 = (u32)kernel_stack + PAGE_SIZE;

  char *proc_name = normal_page_alloc(0);
  memcopy((byte *)args, (byte *)proc_name, strlen(args));
  intToAscii(rand() % 100, &proc_name[6]);

  user_process->name = proc_name;
  /*
    kprintf("Created PID %d\n", user_process->pid);
    kprintf("       Proc name %s\n", (u32)proc_name); */

  UNUSED(data);
  return user_process;
}

Proc *create_kernel_proc(void (*procfunc)(), void *data, char *args, ...) {
  // TODO: cache! chache! cache!
  Proc *user_process = normal_page_alloc(0);

  user_process->isKernelProc = TRUE;

  user_process->p = 0;
  user_process->pid = pid++;
  LIST_INIT(&user_process->head);
  user_process->page_dir = (u32 **)&kernel_page_directory;
  user_process->Vm = kernel_vm;
  user_process->regs.eip = (u32)procfunc;

  void *user_stack = normal_page_alloc(0);
  user_process->regs.esp = (u32)user_stack + PAGE_SIZE - (5 * sizeof(u32));
  ((u32 *)user_process->regs.esp)[4] = (u32)procfunc;

  user_process->stack = user_stack;
  user_process->regs.ds = 0x10;
  user_process->regs.cs = 0x08;
  user_process->regs.ss = 0x10;
  void *kernel_stack = normal_page_alloc(0);
  user_process->kernel_stack_top = kernel_stack;
  user_process->esp0 = (u32)kernel_stack + PAGE_SIZE - (5 * sizeof(u32));

  char *proc_name = normal_page_alloc(0);
  memcopy((byte *)args, (byte *)proc_name, strlen(args));
  intToAscii(rand() % 100, &proc_name[6]);

  user_process->name = proc_name;
  user_process->sched_count = 0;
  user_process->running_ticks = 0;
  /*
    kprintf("Created PID %d\n", user_process->pid);
    kprintf("       Proc name %s\n", (u32)proc_name); */

  UNUSED(data);
  return user_process;
}

void kill_process(Proc *p) {
   if(p->pid == IDLE_PID)
    return;
  list_remove(&p->head);
  // kprintf("\nKilling PID %d\n", p->pid);
  /*  kprintf("      Freeing name pointer(0x%x)\n", (u32)(p->name)); */
  kfreeNormal((void *)p->name);
  /*   kprintf("      Freeing stack pointer(0x%x)\n", (u32)p->stack); */
  kfreeNormal(p->stack);
  kfreeNormal(p->kernel_stack_top);
  /*  kprintf("      Freeing proc(0x%x)\n", (u32)p); */
  kfreeNormal((void *)p);
  // current_proc = NULL;
}