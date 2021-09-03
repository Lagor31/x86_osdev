#ifndef SIGNALS_H
#define SIGNALS_H

#include "../cpu/types.h"
typedef struct Thread Thread;

typedef struct signals_t {
  u32 pending;
  u32 active;
} Signals;

void init_signals(Signals *sigs);
u32 sys_kill(u32 pid, u32 signal);
bool handle_signals(Thread * t);
#define SIG_NUM 32
#define ALL_SIGNALS 0xFFFFFFFF

#define SIGKILL 1 << 9

#endif