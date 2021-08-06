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
  current_thread->sleep_timer = millis_to_ticks(ms) + tick_count;
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
  _switch_to_thread((Thread *)do_schedule());

  // unlock(sched_lock);
}

void sleep_thread(Thread *p) {
  if (p->pid == IDLE_PID) return;
  // kprintf("Sleeping process PID %d\n", p->pid);
  // get_lock(sched_lock);
  list_remove(&p->head);
  list_add(&sleep_queue, &p->head);
  _switch_to_thread((Thread *)do_schedule());

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
  kfree_normal((void *)p->name);
  /*   kprintf("      Freeing stack pointer(0x%x)\n", (u32)p->stack); */
  kfree_normal(p->tcb.user_stack_bot);
  kfree_normal(p->tcb.kernel_stack_bot);
  /*  kprintf("      Freeing proc(0x%x)\n", (u32)p); */
  kfree_normal((void *)p);
  // current_proc = NULL;
}

void printProcSimple(Thread *p) {
  kprintf("%s - PID: %d - N: %d T: %dms\n", p->name, p->pid, p->nice,
          ticks_to_millis(p->runtime));
}

void init_kernel_proc() {
  LIST_INIT(&sleep_queue);
  LIST_INIT(&running_queue);
  LIST_INIT(&stopped_queue);
  idle_thread = create_kernel_thread(idle, NULL, "idle");
  idle_thread->pid = IDLE_PID;
  idle_thread->nice = MIN_PRIORITY;
  idle_thread->sched_count = ticks_to_millis(MIN_QUANTUM_MS);
  wake_up_thread(idle_thread);
  current_thread = idle_thread;
}

void set_kernel_esp(u32 *kesp, u32 entry_point) {
  kesp[7] = 0x200;        // FLAGS
  kesp[6] = KERNEL_CS;    // CS
  kesp[5] = entry_point;  // EIP
  kesp[4] = 0;            // EBP
  kesp[3] = 0;            // ESI
  kesp[2] = 0;            // EDI
  kesp[1] = 0;            // EDX
  kesp[0] = KERNEL_DS;    // DS
}

void set_user_esp(u32 *uesp, u32 entry_point, u32 user_stack) {
  uesp[9] = USER_DS;      // SS
  uesp[8] = user_stack;   // ESP
  uesp[7] = 0x3200;       // FLAGS
  uesp[6] = USER_CS;      // CS
  uesp[5] = entry_point;  // EIP
  uesp[4] = 0;            // EBP
  uesp[3] = 0;            // ESI
  uesp[2] = 0;            // EDI
  uesp[1] = 0;            // EDX
  uesp[0] = USER_DS;      // DS
}

Thread *create_user_thread(void (*entry_point)(), void *data, char *args, ...) {
  // TODO: cache! chache! cache!
  Thread *user_thread = normal_page_alloc(0);

  user_thread->ring0 = FALSE;

  user_thread->nice = 0;
  user_thread->pid = pid++;
  LIST_INIT(&user_thread->head);
  user_thread->tcb.page_dir = PA((u32)user_page_directory);
  user_thread->Vm = kernel_vm;

  void *user_stack = normal_page_alloc(0);
  user_thread->tcb.esp =
      (u32)user_stack + PAGE_SIZE - (U_ESP_SIZE * sizeof(u32));

  set_user_esp((u32 *)user_thread->tcb.esp, (u32)entry_point,
               (u32)(user_stack + PAGE_SIZE));
  user_thread->tcb.user_stack_bot = user_stack;

  void *kernel_stack = normal_page_alloc(0);
  user_thread->tcb.kernel_stack_bot = kernel_stack;
  user_thread->tcb.tss = (u32)kernel_stack + PAGE_SIZE;

  char *proc_name = normal_page_alloc(0);
  u32 name_length = strlen(args);
  memcopy((byte *)args, (byte *)proc_name, name_length);
  proc_name[name_length] = '\0';

  user_thread->sleeping_lock = NULL;
  user_thread->sleep_timer = 0;

  user_thread->name = proc_name;
  user_thread->sched_count = 0;
  user_thread->runtime = 0;

  UNUSED(data);
  return user_thread;
}

Thread *create_kernel_thread(void (*entry_point)(), void *data, char *args,
                             ...) {
  // TODO: cache! chache! cache!
  Thread *kernel_thread = normal_page_alloc(0);

  kernel_thread->ring0 = TRUE;

  kernel_thread->nice = 0;
  kernel_thread->pid = pid++;
  LIST_INIT(&kernel_thread->head);
  kernel_thread->tcb.page_dir = PA((u32)kernel_page_directory);
  kernel_thread->Vm = kernel_vm;

  void *user_stack = normal_page_alloc(0);

  kernel_thread->tcb.esp =
      (u32)user_stack + PAGE_SIZE - (K_ESP_SIZE * sizeof(u32));

  set_kernel_esp((u32 *)kernel_thread->tcb.esp, (u32)entry_point);
  kernel_thread->tcb.user_stack_bot = user_stack;

  void *kernel_stack = normal_page_alloc(0);
  kernel_thread->tcb.kernel_stack_bot = kernel_stack;
  kernel_thread->tcb.tss = (u32)user_stack + PAGE_SIZE;

  char *proc_name = normal_page_alloc(0);
  u32 name_length = strlen(args);
  memcopy((byte *)args, (byte *)proc_name, name_length);
  proc_name[name_length] = '\0';

  kernel_thread->sleeping_lock = NULL;
  kernel_thread->sleep_timer = 0;

  kernel_thread->name = proc_name;
  kernel_thread->sched_count = 0;
  kernel_thread->runtime = 0;
  /*
    kprintf("Created PID %d\n", user_process->pid);
    kprintf("       Proc name %s\n", (u32)proc_name); */

  UNUSED(data);
  return kernel_thread;
}
