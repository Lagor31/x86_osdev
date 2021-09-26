#include "syscall.h"
#include "../drivers/screen.h"
#include "../proc/thread.h"
#include "../lib/utils.h"
#include "signals.h"

void sys_exit(u32 ret_code) {
  Thread *p = current_thread;
  // stop_thread(p);
  kill_process(p);
  p->exit_value = ret_code;
  reschedule();
}

u32 sys_wait4(u32 pid) {
  Thread *p = get_thread(pid);
  if (p == NULL) return -1;
  if (p->state == TASK_STOPPED || p->state == TASK_ZOMBIE) goto done;
  Thread *t = current_thread;
  t->wait4 = pid;
  sleep_thread(current_thread);
  reschedule();
done:
  current_thread->wait4 = 0;
  p->state = TASK_ZOMBIE;
  wake_up_thread(current_thread);
  return p->exit_value;
}

void sys_wait4all() {
  List *l;

  bool all_ko = TRUE;
  list_for_each(l, &current_thread->children) {
    Thread *p1 = (Thread *)list_entry(l, Thread, siblings);
    if (p1->state == TASK_STOPPED || p1->state == TASK_ZOMBIE) continue;
    all_ko = FALSE;
    break;
  }

  if (all_ko) goto done;
  current_thread->wait4child = TRUE;

  sleep_thread(current_thread);
  reschedule();
done:
  current_thread->wait4child = FALSE;
  list_for_each(l, &current_thread->children) {
    Thread *p1 = (Thread *)list_entry(l, Thread, siblings);
    if (p1->state == TASK_STOPPED) p1->state = TASK_ZOMBIE;
  }
}

void sys_printf(char *s) {
  kprintf(s);
  // kprintf("PID: %d\n", current_thread->pid);
}

u32 sys_write(u32 fd, byte *buf, size_t len) {
  UNUSED(fd);
  UNUSED(len);
  u32 i = 0;

  // char c = buf[0];
  // kprintf((const char *)buf);
  while (i < len) {
    write_byte_block(stdout, buf[i]);
    ++i;
  }

  return 0;
}

void get_pid_t() { sys_exit(current_thread->father->pid); }

u32 sys_getpid() {
  MemDesc *thread_mem;
  thread_mem = current_thread->father->mem;
  // thread_mem->page_directory = current_thread->father->mem->page_directory;
  // init_user_paging((u32 *)thread_mem->page_directory);

  Thread *t =
      create_user_thread(get_pid_t, thread_mem, NULL, NULL, "sys_getpid");

  sys_wait4(t->pid);
  return (u32)t->exit_value;
}

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
      sys_printf((char *)regs->ebx);
      break;
    case SYS_SLEEPMS:
      sys_sleepms(regs->ebx);
      break;
    case SYS_WAIT4ALL:
      sys_wait4all();
      break;
    case SYS_WAIT4:
      regs->eax = sys_wait4(regs->ebx);
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
      regs->eax =
          sys_clone((void *)regs->eip + 2, (void *)regs->esp, regs->ebx);
      break;
    case SYS_KILL:
      regs->eax = sys_kill(regs->ebx, regs->ecx);
      break;
    default:
      break;
  }
  regs->eip += 2;
}
