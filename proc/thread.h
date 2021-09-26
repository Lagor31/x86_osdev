#ifndef THREAD_H
#define THREAD_H

#define IDLE_PID 0
#define MIN_PRIORITY 20
#define MAX_PRIORITY 0
#define MAX_QUANTUM_MS 200
#define MIN_QUANTUM_MS 1
#define P_PENALTY 5

#define CLONE_FILES 1
#define CLONE_VM 2

#include "../kernel/files.h"
#include "../lock/lock.h"
#include "../mem/paging.h"
#include "../mem/vma.h"
#include "../lib/list.h"
#include "../lib/constants.h"
#include "../users/user.h"
#include "../mem/mem_desc.h"
#include "../kernel/elf.h"
#include "../kernel/signals.h"

#define TASK_RUNNABLE 0
#define TASK_UNINTERRUPTIBLE 1
#define TASK_INTERRUPTIBLE 2
#define TASK_ZOMBIE 3
#define TASK_STOPPED 4

#define NO_RET_VAL 0xFFFFFFFF

/*
  Thread control block
*/
typedef struct thread_cb {
  u32 esp;
  u32 tss;
  u32 page_dir;
  u32 ret_value;

  void *user_stack_bot;
  void *kernel_stack_bot;

} TCB;

typedef struct Thread Thread;
typedef struct FD FD;

struct Thread {
  TCB tcb; /* Needs to be first to make math easier in asm */
  char *command;
  u32 state;
  u16 pid;
  u16 tgid;
  bool ring0;
  MemDesc *mem;
  Signals signals;
  u8 nice;
  unsigned long long runtime;
  unsigned long long last_activation;
  u32 timeslice;
  Lock *sleeping_lock;
  List waitq;
  u8 wait_flags;
  List head;
  List k_proc_list;
  List files;
  FD *std_files[3];
  Thread *father;
  User *owner;
  List siblings;
  List children;
  bool wait4child;
  u32 wait4;
  u32 exit_value;
};

typedef struct work_task {
  u8 type;
  Thread *t;
  List work_queue;
} Work;

extern Thread *current_thread;
extern Thread *idle_thread;
extern Thread *init_thread;
extern Thread *kwork_thread;
extern List running_queue;
extern List stopped_queue;
extern List sleep_queue;
extern List kwork_queue;
extern List k_threads;
extern u32 pid;

extern void _switch_to_thread(Thread *);

Thread *create_kernel_thread(void (*entry_point)(), void *data, char *args,
                             ...);
Thread *create_user_thread(void (*entry_point)(), MemDesc *mem, void *data,
                           void *stack, char *args, ...);

void printProc(Thread *);
void printTop();
void work_queue_thread();
void init_work_queue();

u32 sys_clone(void (*entry)(), void *stack_ptr, u32 flags);
void set_user_esp(u32 *uesp, u32 entry_point, u32 user_stack);

void printProcSimple(Thread *);
void init_kernel_proc();
void kill_process(Thread *);
void idle();
void load_current_proc(Thread *p);
void wake_up_thread(Thread *p);
void sleep_thread(Thread *p);
void stop_thread(Thread *);
void yield();
void sleep_on_lock(Thread *t, Lock *l);
Thread *get_thread(u32 pid);
void sleep_ms(u32 ms);
void reparent(Thread *adopter, List *adoptees);

#endif