#ifndef PROC_H
#define PROC_H

#define IDLE_PID 0
#define MIN_PRIORITY 20
#define MAX_PRIORITY 0

#include "../utils/list.h"
#include "../mem/vma.h"
#include "../mem/paging.h"

void top();
void do_schedule();

typedef struct process {
  const char *name;
  registers_t regs;
  u32 esp0;
  void *stack;
  u16 pid;
  bool isKernelProc;
  VMRegion *Vm;
  void *kernel_stack_top;
  u32 **page_dir;
  u8 p;
  List head;
} Proc;

extern Proc *current_proc;
extern Proc *idle_proc;

extern void _switch_to_task(Proc *);

void printProc(Proc *);
void init_kernel_proc();
void kill_process(Proc *);
int idle();
void load_current_proc(Proc *p);
void wake_up_process(Proc *p);
void stop_process(Proc *);
void k_simple_proc1();
void k_simple_proc2();

void u_simple_proc();

Proc *create_kernel_proc(int (*procfunc)(void *input), void *data,
                         const char *args, ...);
Proc *create_user_proc(int (*procfunc)(void *input), void *data,
                       const char *args, ...);

#endif