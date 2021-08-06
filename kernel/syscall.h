#ifndef SYSCALL_H
#define SYSCALL_H

#include "../cpu/types.h"
#include "../kernel/scheduler.h"
#include "../lib/utils.h"

#define EXIT 1
#define PRINTF 2
#define WAIT 3

extern void _syscall(u32 num);

void syscall_handler(registers_t *regs);
void sys_exit(u32 ret_code);
void sys_wait(u32 millis);

#endif