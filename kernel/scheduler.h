#ifndef SCHEDULER_H
#define SCHEDULER_H

#include "../proc/thread.h"
#include "../lib/list.h"
#include "../drivers/timer.h"

extern Thread *do_schedule();
extern u32 wake_up_all();

#endif