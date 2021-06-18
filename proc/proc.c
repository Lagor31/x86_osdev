#include "proc.h"
#include "../cpu/timer.h"
#include "../mem/mem.h"
#include "../mem/vma.h"
#include "../cpu/types.h"
#include "../utils/utils.h"
#include "../drivers/screen.h"
#include "../libc/functions.h"
#include "../libc/strings.h"

List sleep_queue;
List running_queue;
List stopped_queue;

Proc *current_proc = NULL;
static u32 pid = IDLE_PID;

void top() {
  List *l;
  Proc *p;

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
    kprintf("[Current]: \n");
    printProcSimple(current_proc);
    resetScreenColors();
  }
}

void stop_process(Proc *p) {
  // kprintf("Stopping process PID %d\n", p->pid);
  list_remove(&p->head);
  list_add(&stopped_queue, &p->head);
  current_proc = NULL;
}

void sleep_process(Proc *p) {
  // kprintf("Sleeping process PID %d\n", p->pid);
  list_remove(&p->head);
  list_add(&sleep_queue, &p->head);
  current_proc = NULL;
}

void u_simple_proc() {
  while (TRUE) {
    u32 r = rand();
    if (r % 1000000 == 0) kprintf("Hello from %s %d\n", current_proc->name, r);
  }
}

void k_simple_proc() {
  while (TRUE) {
    __asm__ __volatile__("hlt");
  }
}

void do_schedule() {
  List *l;
  bool create = (rand() % 300) == 0;
  if (create == TRUE) {
    Proc *n = create_user_proc(&u_simple_proc, NULL, "uproc-aaaa");
    wake_up_process(n);
  }

  if (list_length(&stopped_queue) > 0) {
    list_for_each(l, &stopped_queue) {
      Proc *p = list_entry(l, Proc, head);
      bool kill = (rand() % 10) == 0;
      if (kill == TRUE) {
        kill_process(p);
      }
    }
  }

  if (current_proc != NULL) {
    bool stop = (rand() % 15) == 0;
    if (stop == TRUE && current_proc != NULL && current_proc->pid != IDLE_PID) {
      stop_process(current_proc);
      goto schedule_proc;
    } else {
      bool sleep = (rand() % 2) == 0;
      if (sleep == TRUE && current_proc != NULL &&
          current_proc->pid != IDLE_PID) {
        sleep_process(current_proc);
        goto schedule_proc;
      }
    }
  }

schedule_proc:
  if (list_length(&sleep_queue) > 0) {
    list_for_each(l, &sleep_queue) {
      Proc *p = list_entry(l, Proc, head);
      bool wakeup = (rand() % 2) == 0;
      if (wakeup == TRUE) {
        wake_up_process(p);
        goto end;
      }
    }
  }

  bool c = 0;
  list_for_each(l, &running_queue) {
    Proc *p = list_entry(l, Proc, head);
    // if (c++ == 0) load_current_proc(p);
    if (current_proc != p && p->p <= current_proc->p) {
      load_current_proc(p);
    }
  }
end:
  if (current_proc == NULL) goto schedule_proc;
}

void load_current_proc(Proc *p) { current_proc = p; }
void wake_up_process(Proc *p) {
  // kprintf("Waking up process PID %d\n", p->pid);
  list_remove(&p->head);
  list_add(&running_queue, &p->head);
}

void printProcSimple(Proc *p) {
  kprintf("%s - PID: %d - P: %d\n", p->name, p->pid, p->p);
}
void printProc(Proc *p) {
  kprintf("%s - PID: %d - EIP: %x - ESP: %x - &N: 0x%x - Self: 0x%x\n", p->name,
          p->pid, p->regs.eip, p->regs.esp, p->name, p);
}

void init_kernel_proc() {
  LIST_INIT(&sleep_queue);
  LIST_INIT(&running_queue);
  LIST_INIT(&stopped_queue);
  current_proc = NULL;
  Proc *idle_proc = create_kernel_proc(idle, NULL, "idle");
  idle_proc->pid = IDLE_PID;
  idle_proc->p = MIN_PRIORITY;
  wake_up_process(idle_proc);
  current_proc = idle_proc;
}

int idle() {
  while (TRUE) {
    hlt();
  }
  return -1;
}

Proc *create_user_proc(int (*procfunc)(void *input), void *data,
                       const char *args, ...) {
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
  memcopy(args, proc_name, strlen(args));
  intToAscii(rand() % 100, &proc_name[6]);

  user_process->name = proc_name;
  /*
    kprintf("Created PID %d\n", user_process->pid);
    kprintf("       Proc name %s\n", (u32)proc_name); */

  UNUSED(data);
  return user_process;
}

Proc *create_kernel_proc(int (*procfunc)(void *input), void *data,
                         const char *args, ...) {
  // TODO: cache! chache! cache!
  Proc *kernel_process = normal_page_alloc(0);

  kernel_process->isKernelProc = TRUE;

  kernel_process->p = 0;
  kernel_process->pid = pid++;
  LIST_INIT(&kernel_process->head);
  kernel_process->page_dir = (u32 **)&kernel_page_directory;
  kernel_process->Vm = kernel_vm;
  kernel_process->regs.eip = (u32)procfunc;

  void *kernel_stack = normal_page_alloc(0);
  kernel_process->esp0 = kernel_stack + PAGE_SIZE;
  kernel_process->regs.ds = 0x10;
  kernel_process->regs.cs = 0x08;
  kernel_process->regs.ss = 0x10;

  kernel_process->regs.esp = (u32)kernel_stack + PAGE_SIZE;
  kernel_process->stack = kernel_stack;
  char *proc_name = normal_page_alloc(0);
  memcopy(args, proc_name, strlen(args));
  intToAscii(rand() % 100, &proc_name[6]);

  kernel_process->name = proc_name;

  /*   kprintf("Created PID %d\n", kernel_process->pid);
    kprintf("       Proc struct addr: 0x%x\n", (u32)kernel_process);
    kprintf("       Proc stack addr: 0x%x\n", (u32)kernel_stack);
    kprintf("       Proc name addr: 0x%x\n", (u32)proc_name); */

  UNUSED(data);
  return kernel_process;
}

void kill_process(Proc *p) {
  list_remove(&p->head);
  // kprintf("\nKilling PID %d\n", p->pid);
  /*  kprintf("      Freeing name pointer(0x%x)\n", (u32)(p->name)); */
  kfreeNormal((void *)p->name);
  /*   kprintf("      Freeing stack pointer(0x%x)\n", (u32)p->stack); */
  kfreeNormal(p->stack);
  if (p->isKernelProc == FALSE) kfreeNormal(p->kernel_stack_top);
  /*  kprintf("      Freeing proc(0x%x)\n", (u32)p); */
  kfreeNormal((void *)p);
  current_proc = NULL;
}