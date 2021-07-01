#ifndef LOCK_H
#define LOCK_H

#include "../utils/list.h"

typedef struct spin_lock_t {
  u32 state;
  u32 id;
  List head;
} Lock;

extern Lock *screen_lock;
extern Lock *mem_lock;


#define LOCK_FREE 0
#define LOCK_LOCKED 1

extern void _spin_lock(u32 *state);
extern void _free_lock(u32 *state);
extern u32 _test_spin_lock(u32 *state);

Lock *make_lock();
void init_kernel_locks();
void spin_lock(Lock *);
void unlock(Lock *);
void get_lock(Lock *l);
#endif