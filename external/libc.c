#include "libc.h"

void exit(int exit_code) { _system_call1(SYS_EXIT, exit_code); }

void printf() { _system_call0(SYS_PRINTF); }

void sleepms(unsigned ms) { _system_call1(SYS_SLEEPMS, ms); }

unsigned int random(unsigned int max) { return _system_call1(SYS_RANDOM, max); }