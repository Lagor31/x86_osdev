#include "binaries.h"

void k_simple_proc() {
  int c = 0;
  sleep_ms(rand() % 300);
  while (TRUE) {
    // get_lock(screen_lock);
    /* u32 prevPos = getCursorOffset();
    setCursorPos(current_proc->pid + 1, 50);
    // kprintf("Got lock 0x%x!!!\n", &kernel_spin_lock);
    kprintf("PID: %d N: %d (%d) T: %dms\n", current_proc->pid,
            current_proc->nice, ++c, ticksToMillis(current_proc->runtime));
    // printProcSimple(current_proc);
    setCursorOffset(prevPos); */
    // kprintf("Releasing lock 0x%x :(\n\n", &kernel_spin_lock);

    sleep_ms(rand() % 5 * 1000);

    // unlock(screen_lock);

    // sleep_ms(10);

    // sleep_process(current_proc);
    //_switch_to_task((Proc *)do_schedule());
  }
}
