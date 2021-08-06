#include "binaries.h"

void u_simple_proc() {
  u32 i = 0;
  while (TRUE) {
    //_syscall(55);
    Thread *me = current_thread;
    // disable_int();
    int pos = getCursorOffset();
    setCursorPos(me->pid + 1 % 30, 40);

    // setCursorPos(20, 50);
    kprintf("Usermode PID %d (%d)", me->pid, i++);
    setCursorOffset(pos);
    // enable_int();
    if (i == 30) _syscall(EXIT);
    _syscall(WAIT);
    //_syscall(2);
  }
}