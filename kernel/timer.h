#ifndef KTIMER_H
#define KTIMER_H

#include "../cpu/types.h"
#include "../proc/thread.h"

extern List kernel_timers;
void init_kernel_timers();

typedef struct timer_t {
  u32 expiration; /* In absolute tick counts */
  Thread *thread;
  List q;
} Timer;

#endif