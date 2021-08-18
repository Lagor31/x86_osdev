#include "binaries.h"

void u_child_proc() {
  _setreg(EBX, 2000);
  _syscall(SLEEPMS);
  _setreg(EBX, 0);
  _syscall(EXIT);
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
    // sleep_ms(rand() % 1000);
    _setreg(EBX, 1000);
    _syscall(RANDOM);
    u32 r = getRegisterValue(EAX);
    _setreg(EBX, 1000);
    _syscall(SLEEPMS);
    // kprintf("Creating new uthread!\n");
    /* Thread *t = create_user_thread(u_simple_proc, NULL, "cuser");
    t->nice = 9;
    wake_up_thread(t); */

    /* t = create_user_thread(u_simple_proc, NULL, "cuser");
    t->nice = 9;
    wake_up_thread(t); */
    if (i++ == 5) {
      Thread *t = create_user_thread(u_child_proc, NULL, "child");
      t->nice = 9;
      wake_up_thread(t);

      t = create_user_thread(u_child_proc, NULL, "child");
      t->nice = 9;
      wake_up_thread(t);

      _syscall(WAIT4ALL);
      kprintf("%d -> Children %d exited! Quitting...\n", current_thread->pid,
              t->pid);
      _setreg(EBX, 1000);
      _syscall(SLEEPMS);
      _setreg(EBX, 0);
      _syscall(EXIT);
    }
  }
}