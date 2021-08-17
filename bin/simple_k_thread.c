#include "binaries.h"

void k_child_proc() {
  sleep_ms(rand() % 5000);
  sys_exit(0);
}

void k_simple_proc() {
  sleep_ms(rand() % 300);
  u32 i = 0;
  while (TRUE) {
    // get_lock(screen_lock);
    /*     u32 prevPos = getCursorOffset();
        setCursorPos(current_thread->pid + 1, 50);
        // kprintf("Got lock 0x%x!!!\n", &kernel_spin_lock);

        printProcSimple(current_thread);
        setCursorOffset(prevPos); */
    // kprintf("Releasing lock 0x%x :(\n\n", &kernel_spin_lock);

    sleep_ms(rand() % 5 * 1000);

    if (i++ == 5) {
      Thread *t = create_kernel_thread(k_child_proc, NULL, "k-child");
      t->nice = 9;
      wake_up_thread(t);

      t = create_kernel_thread(k_child_proc, NULL, "k-child");
      t->nice = 9;
      wake_up_thread(t);

      /*  t = create_user_thread(u_simple_proc, NULL, "child");
       t->nice = 9;
       wake_up_thread(t); */

      sys_wait4all();
      kprintf("%d -> Children %d exited! Quitting...\n", current_thread->pid,
              t->pid);
      sys_exit(0);
    }

    // unlock(screen_lock);

    // sleep_ms(10);

    // sleep_process(current_proc);
    //_switch_to_task((Proc *)pick_next_thread());
  }
}
