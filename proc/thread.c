#include "thread.h"

#include "../drivers/timer.h"
#include "../cpu/types.h"
#include "../drivers/screen.h"
#include "../kernel/kernel.h"
#include "../kernel/syscall.h"
#include "../lib/constants.h"
#include "../lib/strings.h"
#include "../mem/mem.h"
#include "../mem/vma.h"
#include "../lib/utils.h"

List sleep_queue;
List running_queue;
List stopped_queue;

Thread *current_thread = NULL;
Thread *idle_thread;
static u32 pid = IDLE_PID;

void sleep_ms(u32 ms) {
  current_thread->sleep_timer = millisToTicks(ms) + tick_count;
  current_thread->sched_count = 0;
  sleep_thread(current_thread);
  _switch_to_thread((Thread *)do_schedule());
}

void stop_thread(Thread *p) {
  if (p->pid == IDLE_PID) return;
  // kprintf("Stopping process PID %d\n", p->pid);
  // get_lock(sched_lock);
  list_remove(&p->head);
  list_add(&stopped_queue, &p->head);
  // unlock(sched_lock);
}

void sleep_thread(Thread *p) {
  if (p->pid == IDLE_PID) return;
  // kprintf("Sleeping process PID %d\n", p->pid);
  // get_lock(sched_lock);
  list_remove(&p->head);
  list_add(&sleep_queue, &p->head);
  // unlock(sched_lock);

  // do_schedule();
  //_switch_to_task(current_proc);
}

void wake_up_thread(Thread *p) {
  // kprintf("Waking up process PID %d\n", p->pid);
  // get_lock(sched_lock);
  list_remove(&p->head);
  list_add(&running_queue, &p->head);
  // unlock(sched_lock);
}

void kill_process(Thread *p) {
  if (p->pid == IDLE_PID) return;
  get_lock(sched_lock);
  list_remove(&p->head);
  unlock(sched_lock);

  // kprintf("\nKilling PID %d\n", p->pid);
  /*  kprintf("      Freeing name pointer(0x%x)\n", (u32)(p->name)); */
  kfreeNormal((void *)p->name);
  /*   kprintf("      Freeing stack pointer(0x%x)\n", (u32)p->stack); */
  kfreeNormal(p->stack);
  kfreeNormal(p->kernel_stack_bot);
  /*  kprintf("      Freeing proc(0x%x)\n", (u32)p); */
  kfreeNormal((void *)p);
  // current_proc = NULL;
}


void printProcSimple(Thread *p) {
  kprintf("%s - PID: %d - N: %d T: %dms\n", p->name, p->pid, p->nice,
          ticksToMillis(p->runtime));
}
void printProc(Thread *p) {
  kprintf("%s - PID: %d - EIP: %x - ESP: %x - &N: 0x%x - Self: 0x%x\n", p->name,
          p->pid, p->regs.eip, p->regs.esp, p->name, p);
}

void init_kernel_proc() {
  LIST_INIT(&sleep_queue);
  LIST_INIT(&running_queue);
  LIST_INIT(&stopped_queue);
  idle_thread = create_kernel_thread(idle, NULL, "idle");
  idle_thread->pid = IDLE_PID;
  idle_thread->nice = MIN_PRIORITY;
  idle_thread->sched_count = ticksToMillis(MIN_QUANTUM_MS);
  wake_up_thread(idle_thread);
  current_thread = idle_thread;
}

Thread *create_user_thread(void (*entry_point)(), void *data, char *args, ...) {
  // TODO: cache! chache! cache!
  Thread *user_process = normal_page_alloc(0);

  user_process->isKernelProc = FALSE;

  user_process->nice = 0;
  user_process->pid = pid++;
  LIST_INIT(&user_process->head);
  user_process->page_dir = PA((u32)user_page_directory);
  user_process->Vm = kernel_vm;
  //user_process->regs.eip = (u32)entry_point;

  void *user_stack = normal_page_alloc(0);
  user_process->regs.esp = (u32)user_stack + PAGE_SIZE - (10 * sizeof(u32));

  // EBP, ESI, EDI, EDX
  ((u32 *)user_process->regs.esp)[9] = 35;                           // SS
  ((u32 *)user_process->regs.esp)[8] = (u32)user_stack + PAGE_SIZE;  // ESP

  // TODO: remove IOPL = 3 (allows usermode threads to run htl, outb and so
  // forth)
  ((u32 *)user_process->regs.esp)[7] = 0x3200;  // flags

  ((u32 *)user_process->regs.esp)[6] = 27;                // CS
  ((u32 *)user_process->regs.esp)[5] = (u32)entry_point;  // EIP

  ((u32 *)user_process->regs.esp)[4] = (u32)0;
  ((u32 *)user_process->regs.esp)[3] = (u32)0;
  ((u32 *)user_process->regs.esp)[2] = (u32)0;
  ((u32 *)user_process->regs.esp)[1] = (u32)0;

  ((u32 *)user_process->regs.esp)[0] = (u32)35;  // DS

  user_process->stack = user_stack;
/*   user_process->regs.ds = 32;
  user_process->regs.cs = 24;
  user_process->regs.ss = 32; */
  void *kernel_stack = normal_page_alloc(0);
  user_process->kernel_stack_bot = kernel_stack;
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

Thread *create_kernel_thread(void (*entry_point)(), void *data, char *args,
                             ...) {
  // TODO: cache! chache! cache!
  Thread *user_process = normal_page_alloc(0);

  user_process->isKernelProc = TRUE;

  user_process->nice = 0;
  user_process->pid = pid++;
  LIST_INIT(&user_process->head);
  user_process->page_dir = PA((u32)kernel_page_directory);
  user_process->Vm = kernel_vm;
  user_process->regs.eip = (u32)entry_point;

  void *user_stack = normal_page_alloc(0);
  user_process->regs.esp = (u32)user_stack + PAGE_SIZE - (8 * sizeof(u32));

  ((u32 *)user_process->regs.esp)[7] = 0x200;  // flags

  ((u32 *)user_process->regs.esp)[6] = 8;                 // CS
  ((u32 *)user_process->regs.esp)[5] = (u32)entry_point;  // EIP

  // EBP, ESI, EDI, EDX
  ((u32 *)user_process->regs.esp)[4] = (u32)0;
  ((u32 *)user_process->regs.esp)[3] = (u32)0;
  ((u32 *)user_process->regs.esp)[2] = (u32)0;
  ((u32 *)user_process->regs.esp)[1] = (u32)0;

  ((u32 *)user_process->regs.esp)[0] = (u32)16;  // DS

  user_process->stack = user_stack;
  user_process->regs.ds = 0x10;
  user_process->regs.cs = 0x08;
  user_process->regs.ss = 0x10;
  void *kernel_stack = normal_page_alloc(0);
  user_process->kernel_stack_bot = kernel_stack;
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

