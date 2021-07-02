#ifndef PROC_H
#define PROC_H

#define IDLE_PID 0
#define MIN_PRIORITY 20
#define MAX_PRIORITY 0
#define MAX_QUANTUM_MS 200
#define MIN_QUANTUM_MS 1
#define P_PENALTY 5

#include "../lock/lock.h"
#include "../mem/paging.h"
#include "../mem/vma.h"
#include "../utils/list.h"

void top();

typedef struct process {
  char *name;
  registers_t regs;
  u32 esp0;
  void *stack;
  u16 pid;
  bool isKernelProc;
  VMRegion *Vm;
  void *kernel_stack_top;
  u32 **page_dir;
  u8 nice;
  u32 runtime;
  u32 sched_count;
  Lock *sleeping_lock;
  u32 sleep_timer;
  List head;
} Proc;

extern Proc *current_proc;
extern Proc *idle_proc;
extern List running_queue;

extern void _switch_to_task(Proc *);

void printProc(Proc *);
void printTop();

void printProcSimple(Proc *);
void init_kernel_proc();
void kill_process(Proc *);
void idle();
void load_current_proc(Proc *p);
void wake_up_process(Proc *p);
void sleep_process(Proc *p);
void stop_process(Proc *);
void k_simple_proc1();
void k_simple_proc2();
void wake_up_all();
void exit(u32 ret_code);

Proc *do_schedule();

void u_simple_proc();
void sleep_ms(u32 ms);

Proc *create_kernel_proc(void (*procfunc)(), void *data, char *args, ...);
Proc *create_user_proc(void (*procfunc)(), void *data, char *args, ...);

#endif