#ifndef THREAD_H
#define THREAD_H

#define IDLE_PID 0
#define MIN_PRIORITY 20
#define MAX_PRIORITY 0
#define MAX_QUANTUM_MS 100
#define MIN_QUANTUM_MS 1
#define P_PENALTY 5

#include "../lock/lock.h"
#include "../mem/paging.h"
#include "../mem/vma.h"
#include "../lib/list.h"


typedef struct thread {
  char *name;
  registers_t regs;
  u32 esp0;
  void *stack;
  u32 page_dir;
  u16 pid;
  bool isKernelProc;
  VMRegion *Vm;
  void *kernel_stack_bot;
  u8 nice;
  u32 runtime;
  u32 sched_count;
  Lock *sleeping_lock;
  u32 sleep_timer;
  List head;
} Thread;

typedef struct work_task{
  char c;
  List work_queue;
} Work;

extern Thread *current_thread;
extern Thread *idle_thread;
extern Thread *kwork_thread;
extern List running_queue;
extern List stopped_queue;
extern List sleep_queue;
extern List kwork_queue;
extern void _switch_to_thread(Thread *);

void printProc(Thread *);
void printTop();
void work_queue_thread();
void init_work_queue();

void printProcSimple(Thread *);
void init_kernel_proc();
void kill_process(Thread *);
void idle();
void load_current_proc(Thread *p);
void wake_up_thread(Thread *p);
void sleep_thread(Thread *p);
void stop_thread(Thread *);

void sleep_ms(u32 ms);

Thread *create_kernel_thread(void (*entry_point)(), void *data, char *args, ...);
Thread *create_user_thread(void (*entry_point)(), void *data, char *args, ...);

#endif