#include "binaries.h"

void top() {
  while (TRUE) {
    get_lock(screen_lock);
    u32 prevCur = getCursorOffset();
    setCursorPos(1, 0);
    // printTop();
    print_tree();
    setCursorOffset(prevCur);
    unlock(screen_lock);
    sleep_ms(500);
  }
}

void print_children(List *children, u32 indent) {
  if (list_length(children) == 0) {
    return;
  }

  List *l;
  Thread *p;
  char pad[50] = "";
  u32 i = 0;
  if (indent > 40) indent = 40;
  for (i = 0; i < indent; ++i) {
    append(pad, ' ');
  }
  pad[i] = '\0';

  list_for_each(l, children) {
    p = list_entry(l, Thread, siblings);
    kprintf("%s- %d - %s\n", pad, p->pid, p->command);
    print_children(&p->children, indent + 1);
  }
}

void print_tree() {
  clearScreen();
  kprintf("- %d - %s\n", init_thread->pid, init_thread->command);
  print_children(&init_thread->children, 1);
}

void printTop() {
  List *l;
  Thread *p;
  Thread *p1;

  u32 c = 0;
  //clearScreen();
  List *children = NULL;

  List *l1;

  list_for_each(l1, &k_threads) {
    p1 = list_entry(l1, Thread, k_proc_list);
    kprintf("- %d - %s\n", p1->pid, p1->command);
    children = &p1->children;

    list_for_each(l, children) {
      p = list_entry(l, Thread, siblings);
      kprintf("   - %d - %s\n", p->pid, p->command);
    }
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
