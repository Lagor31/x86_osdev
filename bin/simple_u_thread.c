#include "binaries.h"

void u_simple_proc() {
  u32 i = 0;
  while (TRUE) {
    //_syscall(55);
    //Thread *me = current_thread;
    //disable_int();
    //setCursorPos(me->pid + 1, 50);
  
    //setCursorPos(20, 50);
    //kprintf("Usermode (%d)", i++);
    //setCursorOffset(pos);
    //enable_int();
    if (i == 7000)
      _syscall(1);
    _syscall(3);
    //_syscall(2);
  }
}