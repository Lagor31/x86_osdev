#include "binaries.h"
#include "../kernel/files.h"

char get_thread_state(Thread *t) {
  char s = 'R';
  switch (t->state) {
    case TASK_RUNNABLE:
      s = 'R';
      break;
    case TASK_STOPPED:
      s = 'X';
      break;
    case TASK_UNINSTERRUPTIBLE:
    case TASK_INTERRUPTIBLE:
      s = 'Z';
      break;
    default:
      s = '?';
      break;
  }

  return s;
}
void printProcSimple(Thread *p) {
  u32 files = list_length(&p->files);
  kprintf("%s - PID: %d - N: %d Parent: %d  %c\n", p->command, p->pid, p->nice,
          p->father->pid, get_thread_state(p));
  kprintf("OF: ");
  if (files > 0) {
    List *l;
    list_for_each(l, &p->files) {
      FD *p1 = (FD *)list_entry(l, FD, q);
      kprintf("%s, ", p1->name);
    }
  }

  kprintf("\n");
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
    kprintf("%s- ", pad);
    print_single_thread(p);
    print_children(&p->children, indent + 1);
  }
}

void print_single_thread(Thread *p) {
  if (!p->ring0) setTextColor(GREEN);

  kprintf("%s pid: %d S:(%c) %s\n", p->command, p->pid, get_thread_state(p),
          p->owner->username);
  resetScreenColors();
}

void print_tree() {
  // clearScreen();
  print_single_thread(init_thread);
  print_children(&init_thread->children, 1);
  kprintf("Cycles: %u\n", cycles_passed);
  kprintf(" q to quit\n");
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

*/
}
void ps() {
  Thread *p;
  List *l;
  int c = 0;
  setBackgroundColor(GREEN);
  setTextColor(BLACK);
  kprintf("[RUNNING]\n");
  list_for_each(l, &running_queue) {
    p = list_entry(l, Thread, head);
    kprintf("[%d] ", c++);
    printProcSimple(p);
  }
  resetScreenColors();

  c = 0;
  setBackgroundColor(CYAN);
  setTextColor(BLACK);
  kprintf("[SLEEP]\n");

  list_for_each(l, &sleep_queue) {
    p = list_entry(l, Thread, head);
    kprintf("[%d] ", c++);
    printProcSimple(p);
  }
  resetScreenColors();

  c = 0;
  setBackgroundColor(GRAY);
  setTextColor(RED);
  kprintf("[STOPPED]\n");

  list_for_each(l, &stopped_queue) {
    p = list_entry(l, Thread, head);
    kprintf("[%d] ", c++);
    printProcSimple(p);
  }
  resetScreenColors();

  if (current_thread != NULL) {
    setBackgroundColor(BLACK);
    setTextColor(GREEN);
    kprintf("[CURRENT]: \n");
    printProcSimple(current_thread);
  }
  resetScreenColors();

  sys_exit(0);
}

void draw_thread() {
  clearScreen();

  while (TRUE) {
    // get_lock(screen_lock);
    setCursorPos(1, 0);
    // printTop();
    print_tree();
    // unlock(screen_lock);
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
