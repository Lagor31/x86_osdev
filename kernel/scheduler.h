#ifndef SCHEDULER_H
#define SCHEDULER_H

#include "../proc/thread.h"
#include "../lib/list.h"
#include "../drivers/timer.h"

extern Thread *do_schedule();
extern void wake_up_all();

#endif