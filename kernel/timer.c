#include "timer.h"

List kernel_timers;

void init_kernel_timers() { LIST_INIT(&kernel_timers); }