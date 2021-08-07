#include "binaries.h"

void u_child_proc() {
  _syscall(SLEEPMS);
  _syscall(EXIT);
}

void u_simple_proc() {
  // u32 i = 0;
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

    _syscall(SLEEPMS);

    // kprintf("Creating new uthread!\n");
    /* Thread *t = create_user_thread(u_simple_proc, NULL, "cuser");
    t->nice = 9;
    wake_up_thread(t); */

    /* t = create_user_thread(u_simple_proc, NULL, "cuser");
    t->nice = 9;
    wake_up_thread(t); */
    if (rand() % 10 == 0) {
      Thread *t = create_user_thread(u_child_proc, NULL, "child");
      t->nice = 9;
      wake_up_thread(t);

      t = create_user_thread(u_child_proc, NULL, "child");
      t->nice = 9;
      wake_up_thread(t);

     /*  t = create_user_thread(u_simple_proc, NULL, "child");
      t->nice = 9;
      wake_up_thread(t); */

      _syscall(WAIT4ALL);
      /* kprintf("%d -> Children %d exited! Quitting...\n", current_thread->pid,
              t->pid); */
      _syscall(SLEEPMS);
      _syscall(EXIT);
    }
    //_syscall(2);
  }
}