#include "binaries.h"

void gui() {
  while (TRUE) {
    u8 prevTextColor = textColor;

    int totFree = total_used_memory / 1024 / 1024;
    int tot = boot_mmap.total_pages * 4096 / 1024 / 1024;

    const char *title =
        " Uptime: %4ds             Used: %4d / %4d Mb"
        "               ProcsRunning: %3d ";
    disable_int();
    u32 prevPos = getCursorOffset();

    setCursorPos(0, 0);
    setBackgroundColor(BLUE);
    setTextColor(YELLOW);
    kprintf(title, get_uptime() / 1000, totFree, tot,
            list_length(&running_queue));

    setCursorOffset(prevPos);
    textColor = prevTextColor;
    enable_int();
    sleep_ms(200);
  }
}
