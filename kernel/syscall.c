#include "syscall.h"
#include "../drivers/screen.h"
#include "../proc/proc.h"

void sys_exit(u32 ret_code) {
  Proc *p = current_proc;
  stop_process(p);
  kill_process(p);
  _switch_to_task(do_schedule());
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
    kprintf("Stopping process PID: %d\n", current_proc->pid);
    sys_exit(regs->ebx);
    break;
  case 2:
    sys_printf(regs->esp);
    regs->eax = 3131;
    break;
  case 3:
    sys_wait(1000);
    break;
  default:
    break;
  }
  regs->eip += 2;
}
