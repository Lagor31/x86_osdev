#include "syscall.h"
#include "../drivers/screen.h"
#include "../proc/thread.h"
#include "../lib/utils.h"

void sys_exit(u32 ret_code) {
  Thread *p = current_thread;
  stop_thread(p);
  kill_process(p);
  p->exit_value = ret_code;
  _switch_to_thread((Thread *)do_schedule());
}

void sys_wait4(u32 pid) {
  Thread *p = current_thread;
  p->wait4 = pid;
  sleep_thread(current_thread);
  _switch_to_thread((Thread *)do_schedule());
}

void sys_wait4all() {
  sleep_thread(current_thread);
  current_thread->wait4child = TRUE;
  _switch_to_thread((Thread *)do_schedule());
}

void sys_printf(u32 number) { kprintf("Input: %x\n", number); }

void sys_sleepms(u32 millis) { sleep_ms(millis); }

void syscall_handler(registers_t *regs) {
  u32 syscall_num = regs->eax;
  // kprintf("Called syscall %d!\n", syscall_num);
  switch (syscall_num) {
    case EXIT:
      // kprintf("Stopping process PID: %d\n", current_thread->pid);
      sys_exit(rand() % 5);
      break;
    case PRINTF:
      sys_printf(regs->esp);
      break;
    case SLEEPMS:
      sys_sleepms((rand() % 5 + 1) * 1000);
      break;
    case WAIT4ALL:
      sys_wait4all();
      break;
    case WAIT4:
      sys_wait4(regs->eax);
      break;
    default:
      break;
  }
  regs->eip += 2;
}
