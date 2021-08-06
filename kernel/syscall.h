#ifndef SYSCALL_H
#define SYSCALL_H

#include "../cpu/types.h"
#include "../kernel/scheduler.h"
#include "../lib/utils.h"

extern void _syscall(u32 num);

void syscall_handler(registers_t *regs);
void sys_exit(u32 ret_code);
void sys_wait(u32 millis);
#endif