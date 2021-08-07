#ifndef SYSCALL_H
#define SYSCALL_H

#include "../cpu/types.h"
#include "../kernel/scheduler.h"
#include "../lib/utils.h"

#define EXIT 1
#define PRINTF 2
#define SLEEPMS 3
#define WAIT4ALL 4
#define WAIT4 5

extern void _syscall(u32 num);

void syscall_handler(registers_t *regs);
void sys_exit(u32 ret_code);
void sys_sleepms(u32 millis);
void sys_wait4all();
void sys_wait4(u32 pid);
#endif