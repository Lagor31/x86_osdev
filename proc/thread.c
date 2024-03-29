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
#include "../kernel/files.h"
#include "../kernel/timer.h"
#include "../mem/mem_desc.h"
#include "../kernel/elf.h"
#include "../kernel/signals.h"
#include "../lib/list.h"

List sleep_queue;
List running_queue;
List stopped_queue;

List k_threads;

Thread *current_thread = NULL;
Thread *idle_thread;
Thread *init_thread;

u32 pid = IDLE_PID;

Thread *get_thread(u32 pid) {
  List *l;
  List *temp;
  Thread *t;
  bool pi = disable_int();
  list_for_each_safe(l, temp, &k_threads) {
    t = list_entry(l, Thread, k_proc_list);
    if (t->pid == pid) {
      enable_int(pi);
      return t;
    }
  }
  enable_int(pi);
  return NULL;
}
void reparent(Thread *adopter, List *adoptees) {
  List *l;
  List *tem;
  adopter = init_thread;
  list_for_each_safe(l, tem, adoptees) {
    Thread *p1 = (Thread *)list_entry(l, Thread, siblings);
    // LIST_INIT(&p1->siblings);
    // kprintf("%d ", p1->pid);
    list_remove(&p1->siblings);

    p1->father = adopter;
    list_add_head(&adopter->children, &p1->siblings);
    // goto redo;
  }
  // kprintf("\n");
}
void sleep_ms(u32 ms) {
  bool pi = disable_int();
  Timer *t = fmalloc(sizeof(Timer));
  t->expiration = millis_to_ticks(ms) + tick_count;
  t->thread = current_thread;
  list_add_head(&kernel_timers, &t->q);
  enable_int(pi);
  sleep_thread(current_thread);
  reschedule();
}

void stop_thread(Thread *p) {
  if (p->pid == IDLE_PID) return;
  bool pi = disable_int();
  p->state = TASK_STOPPED;

  list_remove(&p->head);
  list_add_head(&stopped_queue, &p->head);
  enable_int(pi);
}

void sleep_on_lock(Thread *t, Lock *l) {
  t->sleeping_lock = l;
  sleep_thread(t);
}

void sleep_thread(Thread *p) {
  if (p->pid == IDLE_PID) return;
  bool pi = disable_int();
  p->state = TASK_INTERRUPTIBLE;

  list_remove(&p->head);
  list_add_head(&sleep_queue, &p->head);
  enable_int(pi);
}

void wake_up_thread(Thread *p) {
  bool pi = disable_int();
  p->state = TASK_RUNNABLE;

  list_remove(&p->head);
  list_add_tail(&running_queue, &p->head);
  enable_int(pi);
}

void kill_process(Thread *p) {
  if (p->pid == IDLE_PID) return;

  stop_thread(p);
  //  list_remove(&p->k_proc_list);

  List *l;
  // List *tem;
  /*   if (p->father->wait4child) {
      wake_up_thread(p->father);
      p->father->wait4child = FALSE;
    } */
  // bool wake_up_parent = TRUE;
  List *temp;
  bool pi = disable_int();

  list_for_each_safe(l, temp, &kernel_timers) {
    Timer *activeT = list_entry(l, Timer, q);
    if (activeT->thread->pid == p->pid) {
      list_remove(&activeT->q);
      ffree(activeT);
      break;
    }
  }
  enable_int(pi);

  child_awoken(p->father->pid);
  // wake_up_thread(p->father);

  /* if (p->father->wait4child) {
    list_for_each(l, &p->father->children) {
      Thread *p1 = (Thread *)list_entry(l, Thread, siblings);
      if (p1->state != TASK_STOPPED) wake_up_parent = FALSE;
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
  } */

  /* list_for_each(l, &p->files) {
    // FD *close_me = list_entry(l, FD, q);

  } */

  // Reparenting
  // redo:

  /*
    Work *w = kmalloc(sizeof(Work));
    w->type = 9;
    w->t = p;
    LIST_INIT(&w->work_queue);
    list_add_tail(&kwork_queue, &w->work_queue); */
}

void init_kernel_proc() {
  init_users();
  init_kernel_timers();
  LIST_INIT(&sleep_queue);
  LIST_INIT(&running_queue);
  LIST_INIT(&stopped_queue);
  LIST_INIT(&k_threads);

  // wake_up_thread(idle_thread);

  init_thread = create_kernel_thread(init, NULL, "initd");
  init_thread->nice = MAX_PRIORITY;
  init_thread->pid = 1;
  init_thread->timeslice = ticks_to_millis(MAX_QUANTUM_MS);
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
  uesp[7] = 0x200;        // FLAGS
  uesp[6] = USER_CS;      // CS
  uesp[5] = entry_point;  // EIP
  uesp[4] = 0;            // EBP
  uesp[3] = 0;            // ESI
  uesp[2] = 0;            // EDI
  uesp[1] = 0;            // EDX
  uesp[0] = USER_DS;      // DS
}

Thread *create_user_thread(void (*entry_point)(), MemDesc *mem, void *data,
                           void *stack, char *args, ...) {
  Thread *user_thread = kmalloc(sizeof(Thread));

  user_thread->ring0 = FALSE;
  user_thread->father = current_thread;
  user_thread->nice = 0;
  user_thread->pid = pid++;
  LIST_INIT(&user_thread->head);
  user_thread->tcb.page_dir = PA(mem->page_directory);
  user_thread->mem = mem;
  user_thread->wait4child = FALSE;
  void *user_stack = kalloc_page(0);
  memset((byte *)user_stack, 0, PAGE_SIZE);
  user_thread->tcb.user_stack_bot = user_stack;
  user_thread->tcb.esp =
      (u32)user_stack + PAGE_SIZE - (U_ESP_SIZE * sizeof(u32));
  user_thread->tcb.ret_value = NO_RET_VAL;

  // TODO: is it correct to ignore passed param stack?
  set_user_esp((u32 *)user_thread->tcb.esp, (u32)entry_point,
               stack != NULL ? (u32)stack : USER_STACK_TOP);

  // set_user_esp((u32 *)user_thread->tcb.esp, (u32)entry_point,
  // USER_STACK_TOP);

  void *kernel_stack = kalloc_page(0);
  memset((byte *)kernel_stack, 0, PAGE_SIZE);

  user_thread->tcb.kernel_stack_bot = kernel_stack;
  user_thread->tcb.tss = (u32)kernel_stack + PAGE_SIZE;
  u32 name_length = strlen(args);

  char *proc_name = kmalloc(name_length + 1);
  memcopy((byte *)args, (byte *)proc_name, name_length);
  proc_name[name_length] = '\0';

  init_signals(&user_thread->signals);

  user_thread->command = proc_name;
  user_thread->timeslice = 0;
  user_thread->runtime = 0;
  user_thread->last_activation = 0;
  user_thread->tgid = user_thread->pid;
  user_thread->state = TASK_RUNNABLE;
  LIST_INIT(&user_thread->children);
  LIST_INIT(&user_thread->siblings);
  LIST_INIT(&user_thread->waitq);
  user_thread->wait_flags = 1;
  LIST_INIT(&user_thread->k_proc_list);
  LIST_INIT(&user_thread->files);

  if (current_thread != NULL) {
    user_thread->std_files[0] = current_thread->std_files[0];
    user_thread->std_files[1] = current_thread->std_files[1];
    user_thread->std_files[2] = current_thread->std_files[2];
  } else {
    user_thread->std_files[0] = stdin;
    user_thread->std_files[1] = stdout;
    user_thread->std_files[2] = stderr;
  }

  bool pi = disable_int();
  list_add_head(&k_threads, &user_thread->k_proc_list);
  enable_int(pi);

  if (current_thread != NULL) {
    user_thread->owner = current_thread->owner;
    list_add_head(&current_thread->children, &user_thread->siblings);
  }

  UNUSED(data);
  return user_thread;
}

Thread *create_kernel_thread(void (*entry_point)(), void *data, char *args,
                             ...) {
  Thread *kernel_thread = kmalloc(sizeof(Thread));

  kernel_thread->ring0 = TRUE;
  kernel_thread->father = current_thread;
  kernel_thread->nice = 0;
  kernel_thread->pid = pid++;
  LIST_INIT(&kernel_thread->head);
  kernel_thread->tcb.page_dir = PA((u32)kernel_page_directory);
  kernel_thread->mem = kernel_mem;
  kernel_thread->wait4child = FALSE;
  kernel_thread->tcb.ret_value = NO_RET_VAL;

  void *user_stack = kalloc_page(0);
  memset((byte *)user_stack, 0, PAGE_SIZE);

  kernel_thread->tcb.esp =
      (u32)user_stack + PAGE_SIZE - (K_ESP_SIZE * sizeof(u32));

  set_kernel_esp((u32 *)kernel_thread->tcb.esp, (u32)entry_point);
  kernel_thread->tcb.user_stack_bot = user_stack;

  void *kernel_stack = kalloc_page(0);
  memset((byte *)kernel_stack, 0, PAGE_SIZE);

  kernel_thread->tcb.kernel_stack_bot = kernel_stack;
  kernel_thread->tcb.tss = (u32)user_stack + PAGE_SIZE;

  u32 name_length = strlen(args);
  char *proc_name = kmalloc(name_length + 1);
  memcopy((byte *)args, (byte *)proc_name, name_length);
  proc_name[name_length] = '\0';

  init_signals(&kernel_thread->signals);

  kernel_thread->command = proc_name;
  kernel_thread->timeslice = 0;
  kernel_thread->runtime = 0;
  kernel_thread->last_activation = 0;

  kernel_thread->tgid = kernel_thread->pid;
  LIST_INIT(&kernel_thread->children);
  LIST_INIT(&kernel_thread->siblings);
  LIST_INIT(&kernel_thread->waitq);
  kernel_thread->wait_flags = 1;
  LIST_INIT(&kernel_thread->k_proc_list);
  LIST_INIT(&kernel_thread->files);

  if (current_thread != NULL) {
    kernel_thread->std_files[0] = current_thread->std_files[0];
    kernel_thread->std_files[1] = current_thread->std_files[1];
    kernel_thread->std_files[2] = current_thread->std_files[2];
  } else {
    kernel_thread->std_files[0] = stdin;
    kernel_thread->std_files[1] = stdout;
    kernel_thread->std_files[2] = stderr;
  }
  bool pi = disable_int();
  list_add_head(&k_threads, &kernel_thread->k_proc_list);
  enable_int(pi);

  if (current_thread != NULL) {
    kernel_thread->owner = current_thread->owner;
    list_add_head(&current_thread->children, &kernel_thread->siblings);
  }
  kernel_thread->state = TASK_RUNNABLE;
  /*
    kprintf("Created PID %d\n", user_process->pid);
    kprintf("       Proc name %s\n", (u32)proc_name); */

  UNUSED(data);
  return kernel_thread;
}
