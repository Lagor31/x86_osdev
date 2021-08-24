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
  Thread *p = current_thread;
  p->wait4 = pid;
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

void sys_sleepms(u32 millis) { sleep_ms(millis); }
u32 sys_random(u32 max) { return rand() % max; }

void syscall_handler(registers_t *regs) {
  u32 syscall_num = regs->eax;
  // kprintf("Called syscall %d!\n", syscall_num);
  switch (syscall_num) {
    case EXIT:
      sys_exit(regs->ebx);
      break;
    case PRINTF:
      sys_printf(regs->ebx);
      break;
    case SLEEPMS:
      sys_sleepms(regs->ebx);
      break;
    case WAIT4ALL:
      sys_wait4all();
      break;
    case WAIT4:
      sys_wait4(regs->ebx);
      break;
    case RANDOM:
      regs->eax = sys_random(regs->ebx);
      break;
    default:
      break;
  }
  regs->eip += 2;
}
