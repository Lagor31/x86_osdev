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
  void *stack;
  u16 pid;
  bool isKernelProc;
  VMRegion *Vm;
  registers_t regs;
  u32 **page_dir;
  u8 p;
  List head;
} Proc;

void printProc(Proc *);
void init_kernel_proc();
void kill_process(Proc *);
int idle();
void load_current_proc(Proc *p);
void wake_up_process(Proc *p);
void stop_process(Proc *);


Proc *create_kernel_proc(int (*procfunc)(void *input), void *data,
                         const char *args, ...);

#endif