#ifndef SCHEDULER_H
#define SCHEDULER_H

#include "../proc/thread.h"
#include "../lib/list.h"
#include "../drivers/timer.h"
#include "../bin/binaries.h"

extern Thread *pick_next_thread();
extern u32 wake_up_all();
extern void reschedule();

#endif