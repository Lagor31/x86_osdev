#include "syscall.h"
#include "../drivers/screen.h"
#include "../proc/thread.h"
#include "../lib/utils.h"

void sys_exit(u32 ret_code) {
  Thread *p = current_thread;
  stop_thread(p);
  kill_process(p);
  _switch_to_thread(do_schedule());
  UNUSED(ret_code);

}

void sys_printf(u32 number) { kprintf("Input: %x\n", number); }

void sys_wait(u32 millis){
  sleep_ms(millis);
}

void syscall_handler(registers_t *regs) {
  u32 syscall_num = regs->eax;
  //kprintf("Called syscall %d!\n", syscall_num);
  switch (syscall_num) {
  case 1:
    kprintf("Stopping process PID: %d\n", current_thread->pid);
    sys_exit(regs->ebx);
    break;
  case 2:
    sys_printf(regs->esp);
    break;
  case 3:
    sys_wait(rand() % 5 * 1000);
    break;
  default:
    break;
  }
  regs->eip += 2;
}
