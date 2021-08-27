#ifndef SYSCALL_H
#define SYSCALL_H

#include "../cpu/types.h"
#include "../kernel/scheduler.h"
#include "../lib/utils.h"
#include "../proc/thread.h"

#define SYS_EXIT 1
#define SYS_PRINTF 2
#define SYS_SLEEPMS 3
#define SYS_WAIT4ALL 4
#define SYS_WAIT4 5
#define SYS_RANDOM 6
#define SYS_WRITE 7
#define SYS_GETPID 8
#define SYS_CLONE 9

extern u32 _syscall(u32 num);

void syscall_handler(registers_t *regs);
void sys_exit(u32 ret_code);
void sys_sleepms(u32 millis);
void sys_wait4all();
void sys_wait4(u32 pid);
u32 sys_write(u32 fd, byte * src, size_t len);
u32 sys_getpid();

#endif