#ifndef LOCK_H
#define LOCK_H

#include "../lib/list.h"
#include "../kernel/wait_queue.h"

typedef struct Thread Thread;

typedef struct spin_lock_t {
  u32 state;
  u32 id;
  List head;
  Thread *owner;
  WaitQ *wait_q;
} Lock;

extern Lock *screen_lock;
extern Lock *kmem_lock;
extern Lock *nmem_lock;
extern Lock *sched_lock;
extern Lock *work_queue_lock;

#define LOCK_FREE 0
#define LOCK_LOCKED 1

extern void _spin_lock(u32 *state);
extern void _free_lock(u32 *state);
extern u32 _test_spin_lock(u32 *state);

Lock *make_lock();
Lock *make_lock_nosleep();
void init_kernel_locks();
void spin_lock(Lock *);
void unlock(Lock *);
void get_lock(Lock *l);
u32 test_lock(Lock *l);
void disable_int();
void enable_int();
#endif