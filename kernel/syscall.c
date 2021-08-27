#include "syscall.h"
#include "../drivers/screen.h"
#include "../proc/thread.h"
#include "../lib/utils.h"

void sys_exit(u32 ret_code) {
  Thread *p = current_thread;
  stop_thread(p);
  kill_process(p);
  p->exit_value = ret_code;
  reschedule();
}

void sys_wait4(u32 pid) {
  Thread *p = get_thread(pid);
  if (p == NULL) return;
  Thread *t = current_thread;
  t->wait4 = pid;
  sleep_thread(current_thread);
  reschedule();
}

void sys_wait4all() {
  sleep_thread(current_thread);
  current_thread->wait4child = TRUE;
  reschedule();
}

void sys_printf(u32 number) {
  kprintf("PID: %d\n", current_thread->pid);
  UNUSED(number);
}

u32 sys_write(u32 fd, byte *buf, size_t len) {
  UNUSED(fd);
  UNUSED(len);
  // int i = 0;
  // char c = buf[0];
  kprintf((const char *)buf);
  /* while (i++ < len) {
    write_byte_stream(stdout, buf[i]);
  } */

  return 0;
}

u32 sys_getpid() { return (u32)current_thread->pid; }

void sys_sleepms(u32 millis) { sleep_ms(millis); }
u32 sys_random(u32 max) { return rand() % max; }

void syscall_handler(registers_t *regs) {
  u32 syscall_num = regs->eax;
  // kprintf("Called syscall %d!\n", syscall_num);
  switch (syscall_num) {
    case SYS_EXIT:
      sys_exit(regs->ebx);
      break;
    case SYS_PRINTF:
      sys_printf(regs->ebx);
      break;
    case SYS_SLEEPMS:
      sys_sleepms(regs->ebx);
      break;
    case SYS_WAIT4ALL:
      sys_wait4all();
      break;
    case SYS_WAIT4:
      sys_wait4(regs->ebx);
      break;
    case SYS_RANDOM:
      regs->eax = sys_random(regs->ebx);
      break;
    case SYS_WRITE:
      regs->eax = sys_write(regs->ebx, (byte *)regs->ecx, regs->edx);
      break;
    case SYS_GETPID:
      regs->eax = sys_getpid();
      break;
    case SYS_CLONE:
      regs->eax = sys_clone(regs->eip + 2, regs->esp, regs->ebx);
      break;
    default:
      break;
  }
  regs->eip += 2;
}
