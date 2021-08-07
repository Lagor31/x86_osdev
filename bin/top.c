#include "binaries.h"

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
    kprintf("%s- %d (%d)- %s %dms\n", pad, p->pid, p->state, p->command,
            millis_to_ticks(p->runtime));
    print_children(&p->children, indent + 1);
  }
}

void print_tree() {
  // clearScreen();
  kprintf("- %d - (%d) %s %dms\n", init_thread->pid, init_thread->state,
          init_thread->command, init_thread->runtime);
  print_children(&init_thread->children, 1);
}

void printTop() {
  List *l;
  Thread *p;
  Thread *p1;

  // clearScreen();
  List *children = NULL;

  List *l1;

  list_for_each(l1, &k_threads) {
    p1 = list_entry(l1, Thread, k_proc_list);
    kprintf("- %d - %s %d\n", p1->pid, p1->command,
            ticks_to_millis(p1->runtime));
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

void draw_thread() {
  while (TRUE) {
    //get_lock(screen_lock);
    clearScreen();
    setCursorPos(1, 0);
    // printTop();
    print_tree();
    //unlock(screen_lock);
    sleep_ms(1000);
  }
}

void top() {
  Thread *draw = create_kernel_thread(&draw_thread, NULL, "draw");
  draw->nice = 0;
  wake_up_thread(draw);
  while (TRUE) {
    char read = read_stdin();
    if (read == 'q') {
      kill_process(draw);
      sys_exit(EXIT);
    }
  }
}
