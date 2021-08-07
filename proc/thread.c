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
#include "../users/user.h"

List sleep_queue;
List running_queue;
List stopped_queue;

List k_threads;

Thread *current_thread = NULL;
Thread *idle_thread;
Thread *init_thread;

u32 pid = IDLE_PID;

void sleep_ms(u32 ms) {
  current_thread->sleep_timer = millis_to_ticks(ms) + tick_count;
  // current_thread->sched_count = 0;
  sleep_thread(current_thread);
  _switch_to_thread((Thread *)do_schedule());
}

void stop_thread(Thread *p) {
  if (p->pid == IDLE_PID) return;
  // kprintf("Stopping process PID %d\n", p->pid);
  // get_lock(sched_lock);
  p->state = TASK_STOPPED;
  list_remove(&p->head);
  list_add(&stopped_queue, &p->head);
  //_switch_to_thread((Thread *)do_schedule());

  // unlock(sched_lock);
}

void sleep_thread(Thread *p) {
  if (p->pid == IDLE_PID) return;
  // kprintf("Sleeping process PID %d\n", p->pid);
  // get_lock(sched_lock);
  p->state = TASK_INTERRUPTIBLE;
  list_remove(&p->head);
  list_add(&sleep_queue, &p->head);
  //_switch_to_thread((Thread *)do_schedule());

  // unlock(sched_lock);

  // do_schedule();
  //_switch_to_task(current_proc);
}

void wake_up_thread(Thread *p) {
  // kprintf("Waking up process PID %d\n", p->pid);
  p->state = TASK_RUNNABLE;
  // get_lock(sched_lock);
  list_remove(&p->head);
  list_add(&running_queue, &p->head);
  // unlock(sched_lock);
}

void kill_process(Thread *p) {
  if (p->pid == IDLE_PID) return;
  get_lock(sched_lock);
  list_remove(&p->head);
  list_remove(&p->k_proc_list);
  p->state = TASK_ZOMBIE;

  List *l;
  /*   if (p->father->wait4child) {
      wake_up_thread(p->father);
      p->father->wait4child = FALSE;
    } */
  bool wake_up_parent = TRUE;

  if (p->father->wait4child) {
    list_for_each(l, &p->father->children) {
      Thread *p1 = (Thread *)list_entry(l, Thread, siblings);
      if (p1->state != TASK_ZOMBIE) wake_up_parent = FALSE;
    }
  }

  if (p->father->wait4 == p->pid) {
    wake_up_thread(p->father);
    p->father->wait4 = 0;
  } else {
    if (p->father->wait4child && wake_up_parent) {
      wake_up_thread(p->father);
      p->father->wait4child = FALSE;
    }
  }

// kprintf("Dead father had ->\n");
// Reparenting
redo:

  list_for_each(l, &p->children) {
    Thread *p1 = (Thread *)list_entry(l, Thread, siblings);
    // LIST_INIT(&p1->siblings);
    // kprintf("%d ", p1->pid);
    list_remove(&p1->siblings);

    p1->father = p->father;
    list_add(&p->father->children, &p1->siblings);
    goto redo;
  }
  // kprintf("\n");

  list_remove(&p->children);
  list_remove(&p->siblings);

  unlock(sched_lock);

  // kprintf("\nKilling PID %d\n", p->pid);
  /*  kprintf("      Freeing name pointer(0x%x)\n", (u32)(p->name)); */
  // Do separetly
  /*   kfree_normal((void *)p->command);
   */  /*   kprintf("      Freeing stack pointer(0x%x)\n", (u32)p->stack); */
  // Do separetly

  // kfree_normal(p->tcb.user_stack_bot);
  // kfree_normal(p->tcb.kernel_stack_bot);
  /*  kprintf("      Freeing proc(0x%x)\n", (u32)p); */
  // kfree_normal((void *)p);
  // current_proc = NULL;
}

void printProcSimple(Thread *p) {
  char s = 'R';
  switch (p->state) {
    case TASK_RUNNABLE:
      s = 'R';
      break;
    case TASK_STOPPED:
      s = 'X';
      break;
    case TASK_UNINSTERRUPTIBLE:
    case TASK_INTERRUPTIBLE:
      s = 'Z';
      break;
    default:
      s = '?';
      break;
  }
  kprintf("%s - PID: %d - N: %d F: %d T: %dms %c\n", p->command, p->pid,
          p->nice, p->father->pid, ticks_to_millis(p->runtime), s);
  /*  List *l;
   list_for_each(l, &p->children) {
     Thread *p1 = (Thread *)list_entry(l, Thread, siblings);
     kprintf("%s,", p1->command);
   }
   kprintf("\n"); */
}

void init_kernel_proc() {
  init_users();

  LIST_INIT(&sleep_queue);
  LIST_INIT(&running_queue);
  LIST_INIT(&stopped_queue);
  LIST_INIT(&k_threads);

  // wake_up_thread(idle_thread);

  init_thread = create_kernel_thread(init, NULL, "initd");
  init_thread->nice = MAX_PRIORITY;
  init_thread->pid = 1;
  init_thread->sched_count = ticks_to_millis(MAX_QUANTUM_MS);
  init_thread->father = init_thread;
  init_thread->owner = root;

  pid = 2;
  wake_up_thread(init_thread);

  // current_thread = idle_thread;
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
  user_thread->father = current_thread;
  user_thread->nice = 0;
  user_thread->pid = pid++;
  LIST_INIT(&user_thread->head);
  user_thread->tcb.page_dir = PA((u32)user_page_directory);
  user_thread->vm = kernel_vm;
  user_thread->wait4child = FALSE;
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

  user_thread->command = proc_name;
  user_thread->sched_count = 0;
  user_thread->runtime = 0;

  user_thread->tgid = user_thread->pid;
  user_thread->state = TASK_RUNNABLE;
  LIST_INIT(&user_thread->children);
  LIST_INIT(&user_thread->siblings);

  LIST_INIT(&user_thread->k_proc_list);

  list_add(&k_threads, &user_thread->k_proc_list);
  if (current_thread != NULL) {
    user_thread->owner = current_thread->owner;
    list_add(&current_thread->children, &user_thread->siblings);
  }

  UNUSED(data);
  return user_thread;
}

Thread *create_kernel_thread(void (*entry_point)(), void *data, char *args,
                             ...) {
  // TODO: cache! chache! cache!
  Thread *kernel_thread = normal_page_alloc(0);

  kernel_thread->ring0 = TRUE;
  kernel_thread->father = current_thread;
  kernel_thread->nice = 0;
  kernel_thread->pid = pid++;
  LIST_INIT(&kernel_thread->head);
  kernel_thread->tcb.page_dir = PA((u32)kernel_page_directory);
  kernel_thread->vm = kernel_vm;
  kernel_thread->wait4child = FALSE;

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

  kernel_thread->command = proc_name;
  kernel_thread->sched_count = 0;
  kernel_thread->runtime = 0;

  kernel_thread->tgid = kernel_thread->pid;
  LIST_INIT(&kernel_thread->children);
  LIST_INIT(&kernel_thread->siblings);

  LIST_INIT(&kernel_thread->k_proc_list);
  list_add(&k_threads, &kernel_thread->k_proc_list);
  if (current_thread != NULL) {
    kernel_thread->owner = current_thread->owner;
    list_add(&current_thread->children, &kernel_thread->siblings);
  }
  kernel_thread->state = TASK_RUNNABLE;
  /*
    kprintf("Created PID %d\n", user_process->pid);
    kprintf("       Proc name %s\n", (u32)proc_name); */

  UNUSED(data);
  return kernel_thread;
}
