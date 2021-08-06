#include "binaries.h"

void top() {
  while (TRUE) {
    get_lock(screen_lock);
    u32 prevCur = getCursorOffset();
    setCursorPos(1, 0);
    printTop();
    setCursorOffset(prevCur);
    unlock(screen_lock);
    sleep_ms(2000);
  }
}

void printTop() {
  List *l;
  Thread *p;
  u32 c = 0;
  clearScreen();
  list_for_each(l, &k_threads) {
    p = list_entry(l, Thread, k_proc_list);
    kprintf("[%2d] ", c++);
    printProcSimple(p);
  }
  /*
  setBackgroundColor(GREEN);
 setTextColor(BLACK);
 kprintf("[RUNNING]\n");
 disable_int();
 list_for_each(l, &running_queue) {
   p = list_entry(l, Thread, head);
   kprintf("[%d] ", c++);
   printProcSimple(p);
 }
 enable_int();
 resetScreenColors();

 c = 0;
 setBackgroundColor(CYAN);
 setTextColor(BLACK);
 kprintf("[SLEEP]\n");
 disable_int();

 list_for_each(l, &sleep_queue) {
   p = list_entry(l, Thread, head);
   kprintf("[%d] ", c++);
   printProcSimple(p);
 }
 enable_int();
 resetScreenColors();

 c = 0;
 setBackgroundColor(GRAY);
 setTextColor(RED);
 kprintf("[STOPPED]\n");
 disable_int();

 list_for_each(l, &stopped_queue) {
   p = list_entry(l, Thread, head);
   kprintf("[%d] ", c++);
   printProcSimple(p);
 }
 enable_int();
 resetScreenColors();
 disable_int();

 if (current_thread != NULL) {
   setBackgroundColor(BLACK);
   setTextColor(GREEN);
   kprintf("[CURRENT]: \n");
   printProcSimple(current_thread);
 }
 enable_int();
 resetScreenColors();
 */
}
