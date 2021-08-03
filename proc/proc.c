#include "proc.h"
#include "../cpu/timer.h"
#include "../cpu/types.h"
#include "../drivers/screen.h"
#include "../kernel/kernel.h"
#include "../kernel/syscall.h"
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
  while (TRUE) {
    get_lock(screen_lock);
    u32 prevCur = getCursorOffset();
    setCursorPos(1, 0);
    printTop();
    setCursorOffset(prevCur);
    unlock(screen_lock);
    sleep_ms(200);
  }
}

void printTop() {
  List *l;
  Proc *p;

  setBackgroundColor(GREEN);
  setTextColor(BLACK);
  kprintf("[RUNNING]\n");
  u32 c = 0;
  disable_int();
  list_for_each(l, &running_queue) {
    p = list_entry(l, Proc, head);
    kprintf("[%d] ", c++);
    printProcSimple(p);
  }
  enable_int();
  resetScreenColors();

  c = 0;
  setBackgroundColor(CYAN);
  setTextColor(BLACK);
  kprintf("[SLEEP]\n");
  disable_int();

  list_for_each(l, &sleep_queue) {
    p = list_entry(l, Proc, head);
    kprintf("[%d] ", c++);
    printProcSimple(p);
  }
  enable_int();
  resetScreenColors();

  c = 0;
  setBackgroundColor(GRAY);
  setTextColor(RED);
  kprintf("[STOPPED]\n");
  disable_int();

  list_for_each(l, &stopped_queue) {
    p = list_entry(l, Proc, head);
    kprintf("[%d] ", c++);
    printProcSimple(p);
  }
  enable_int();
  resetScreenColors();
  disable_int();

  if (current_proc != NULL) {
    setBackgroundColor(BLACK);
    setTextColor(GREEN);
    kprintf("[CURRENT]: \n");
    printProcSimple(current_proc);
  }
  enable_int();
  resetScreenColors();
}

void sleep_ms(u32 ms) {
  current_proc->sleep_timer = millisToTicks(ms) + tickCount;
  current_proc->sched_count = 0;
  sleep_process(current_proc);
  _switch_to_task((Proc *)do_schedule());
}

void stop_process(Proc *p) {
  if (p->pid == IDLE_PID)
    return;
  // kprintf("Stopping process PID %d\n", p->pid);
  // get_lock(sched_lock);
  list_remove(&p->head);
  list_add(&stopped_queue, &p->head);
  // unlock(sched_lock);
}

void sleep_process(Proc *p) {
  if (p->pid == IDLE_PID)
    return;
  // kprintf("Sleeping process PID %d\n", p->pid);
  // get_lock(sched_lock);
  list_remove(&p->head);
  list_add(&sleep_queue, &p->head);
  // unlock(sched_lock);

  // do_schedule();
    //_switch_to_task(current_proc);
  }


void u_simple_proc() {
  u32 i = 0;
  while (TRUE) {
    //_syscall(55);
    Proc *me = current_proc;
    //disable_int();
    int pos = getCursorOffset();
    setCursorPos(me->pid + 1, 50);
    u32 r = rand();

    //setCursorPos(20, 50);
    kprintf("Usermode (%d)", i++);
    setCursorOffset(pos);
    //enable_int();
    if (i == 7000)
      _syscall(1);
    _syscall(3);
    //_syscall(2);
  }
}

Proc *do_schedule() {
  List *l;

  u32 i = 0;
  u32 pAvg = 0;
  u32 pTot = 0;
  Proc *next = NULL;

  if (list_length(&kwork_queue) > 0)
    return kwork_thread;

  u32 proc_num = list_length(&running_queue);
  if (proc_num > 1) {
    list_for_each(l, &running_queue) {
      Proc *p = (Proc *)list_entry(l, Proc, head);
      if (i++ == 0 && p->sched_count > 0 && p->pid != IDLE_PID)
        return p;
      pTot += p->nice;
    }

    pAvg = pTot / (proc_num - 1);
    i = 0;

    list_for_each(l, &running_queue) {
      Proc *p = (Proc *)list_entry(l, Proc, head);
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
      Proc *p = (Proc *)list_entry(l, Proc, head);
      if (i++ == 0) {
        next = p;
        break;
      }
    }
    return next;
  }

  idle_proc->sched_count = millisToTicks(MIN_QUANTUM_MS);
  return idle_proc;
}

void wake_up_all() {
  List *l;
wake_up:
  if (list_length(&sleep_queue) > 0) {
    list_for_each(l, &sleep_queue) {
      Proc *p = list_entry(l, Proc, head);
      if (p->sleeping_lock != NULL && p->sleeping_lock->state == LOCK_FREE) {
        p->sleeping_lock = NULL;
        wake_up_process(p);
        goto wake_up;

      } else if (p->sleep_timer != 0 && tickCount >= p->sleep_timer) {
        p->sleep_timer = 0;
        wake_up_process(p);
        goto wake_up;
      }
    }
  }
}

void wake_up_process(Proc *p) {
  // kprintf("Waking up process PID %d\n", p->pid);
  // get_lock(sched_lock);
  list_remove(&p->head);
  list_add(&running_queue, &p->head);
  // unlock(sched_lock);
}

void printProcSimple(Proc *p) {
  kprintf("%s - PID: %d - N: %d T: %dms\n", p->name, p->pid, p->nice,
          ticksToMillis(p->runtime));
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
  idle_proc->nice = MIN_PRIORITY;
  idle_proc->sched_count = ticksToMillis(MIN_QUANTUM_MS);
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

  user_process->nice = 0;
  user_process->pid = pid++;
  LIST_INIT(&user_process->head);
  user_process->page_dir = PA((u32)user_page_directory);
  user_process->Vm = kernel_vm;
  user_process->regs.eip = (u32)procfunc;

  void *user_stack = normal_page_alloc(0);
  user_process->regs.esp = (u32)user_stack + PAGE_SIZE - (10 * sizeof(u32));

  // EBP, ESI, EDI, EDX
  ((u32 *)user_process->regs.esp)[9] = 35;                          // SS
  ((u32 *)user_process->regs.esp)[8] = (u32)user_stack + PAGE_SIZE; // ESP

  ((u32 *)user_process->regs.esp)[7] = 0x200; // flags

  ((u32 *)user_process->regs.esp)[6] = 27;            // CS
  ((u32 *)user_process->regs.esp)[5] = (u32)procfunc; // EIP

  ((u32 *)user_process->regs.esp)[4] = (u32)0;
  ((u32 *)user_process->regs.esp)[3] = (u32)0;
  ((u32 *)user_process->regs.esp)[2] = (u32)0;
  ((u32 *)user_process->regs.esp)[1] = (u32)0;

  ((u32 *)user_process->regs.esp)[0] = (u32)35; // DS

  user_process->stack = user_stack;
  user_process->regs.ds = 32;
  user_process->regs.cs = 24;
  user_process->regs.ss = 32;
  void *kernel_stack = normal_page_alloc(0);
  user_process->kernel_stack_top = kernel_stack;
  user_process->esp0 = (u32)kernel_stack + PAGE_SIZE;

  char *proc_name = normal_page_alloc(0);
  u32 name_length = strlen(args);
  memcopy((byte *)args, (byte *)proc_name, name_length);
  proc_name[name_length] = '\0';

  user_process->sleeping_lock = NULL;
  user_process->sleep_timer = 0;

  user_process->name = proc_name;
  user_process->sched_count = 0;
  user_process->runtime = 0;
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

  user_process->nice = 0;
  user_process->pid = pid++;
  LIST_INIT(&user_process->head);
  user_process->page_dir = PA((u32)kernel_page_directory);
  user_process->Vm = kernel_vm;
  user_process->regs.eip = (u32)procfunc;

  void *user_stack = normal_page_alloc(0);
  user_process->regs.esp = (u32)user_stack + PAGE_SIZE - (8 * sizeof(u32));

  ((u32 *)user_process->regs.esp)[7] = 0x200; // flags

  ((u32 *)user_process->regs.esp)[6] = 8;             // CS
  ((u32 *)user_process->regs.esp)[5] = (u32)procfunc; // EIP

  // EBP, ESI, EDI, EDX
  ((u32 *)user_process->regs.esp)[4] = (u32)0;
  ((u32 *)user_process->regs.esp)[3] = (u32)0;
  ((u32 *)user_process->regs.esp)[2] = (u32)0;
  ((u32 *)user_process->regs.esp)[1] = (u32)0;

  ((u32 *)user_process->regs.esp)[0] = (u32)16; // DS

  user_process->stack = user_stack;
  user_process->regs.ds = 0x10;
  user_process->regs.cs = 0x08;
  user_process->regs.ss = 0x10;
  void *kernel_stack = normal_page_alloc(0);
  user_process->kernel_stack_top = kernel_stack;
  user_process->esp0 = (u32)user_stack + PAGE_SIZE;

  char *proc_name = normal_page_alloc(0);
  u32 name_length = strlen(args);
  memcopy((byte *)args, (byte *)proc_name, name_length);
  // intToAscii(rand() % 100, &proc_name[6]);
  proc_name[name_length] = '\0';

  user_process->sleeping_lock = NULL;
  user_process->sleep_timer = 0;

  user_process->name = proc_name;
  user_process->sched_count = 0;
  user_process->runtime = 0;
  /*
    kprintf("Created PID %d\n", user_process->pid);
    kprintf("       Proc name %s\n", (u32)proc_name); */

  UNUSED(data);
  return user_process;
}

void kill_process(Proc *p) {
  if (p->pid == IDLE_PID)
    return;
  get_lock(sched_lock);
  list_remove(&p->head);
  unlock(sched_lock);

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