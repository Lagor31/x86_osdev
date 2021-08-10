#include "binaries.h"

void u_child_proc() {
  sleep_ms(rand() % 300000);
  sys_exit(0);
}

void u_simple_proc() {
  u32 i = 0;
  while (TRUE) {
    //_syscall(55);
    // Thread *me = current_thread;
    // disable_int();
    /* int pos = getCursorOffset();
    setCursorPos(me->pid + 1 % 30, 45);

    // setCursorPos(20, 50);
    kprintf("Usermode PID %d (%d)", me->pid, i++);
    setCursorOffset(pos); */
    // enable_int();
    //sleep_ms(rand() % 1000);

    _syscall(SLEEPMS);
    if (i++ == 20) _syscall(EXIT);
    // kprintf("Creating new uthread!\n");
    /* Thread *t = create_user_thread(u_simple_proc, NULL, "cuser");
    t->nice = 9;
    wake_up_thread(t); */

    /* t = create_user_thread(u_simple_proc, NULL, "cuser");
    t->nice = 9;
    wake_up_thread(t); */
    if (i++ == 20) {
      Thread *t = create_user_thread(u_child_proc, NULL, "child");
      t->nice = 9;
      wake_up_thread(t);

      t = create_user_thread(u_child_proc, NULL, "child");
      t->nice = 9;
      wake_up_thread(t);

      /*  t = create_user_thread(u_simple_proc, NULL, "child");
       t->nice = 9;
       wake_up_thread(t); */

      sys_wait4all();
      /* kprintf("%d -> Children %d exited! Quitting...\n", current_thread->pid,
              t->pid); */
      sleep_ms(2000);

      sys_exit(0);
    }
    //_syscall(2);
  }
}