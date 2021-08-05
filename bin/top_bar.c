
#include "binaries.h"

void top_bar() {
  while (TRUE) {
    // kprintf("1) PID %d\n", current_proc->pid);
    // printProc(current_proc);

    int totFree = total_used_memory / 1024 / 1024;
    int tot = boot_mmap.total_pages * 4096 / 1024 / 1024;

    const char *title =
        " Uptime: %4ds             Used: %4d / %4d Mb"
        "               ProcsRunning: %3d ";
    get_lock(screen_lock);
    u32 prevPos = getCursorOffset();

    setCursorPos(0, 0);
    setBackgroundColor(BLUE);
    setTextColor(YELLOW);
    kprintf(title, getUptime() / 1000, totFree, tot,
            list_length(&running_queue));

    setCursorOffset(prevPos);
    resetScreenColors();

    unlock(screen_lock);
    sleep_ms(100);
  }
}
