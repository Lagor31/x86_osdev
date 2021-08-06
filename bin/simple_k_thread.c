#include "binaries.h"

void k_simple_proc() {
  sleep_ms(rand() % 300);
  while (TRUE) {
    // get_lock(screen_lock);
/*     u32 prevPos = getCursorOffset();
    setCursorPos(current_thread->pid + 1, 50);
    // kprintf("Got lock 0x%x!!!\n", &kernel_spin_lock);

    printProcSimple(current_thread);
    setCursorOffset(prevPos); */
    // kprintf("Releasing lock 0x%x :(\n\n", &kernel_spin_lock);

    sleep_ms(rand() % 5 * 10);

    // unlock(screen_lock);

    // sleep_ms(10);

    // sleep_process(current_proc);
    //_switch_to_task((Proc *)do_schedule());
  }
}
