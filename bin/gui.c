#include "binaries.h"
#include "../drivers/cmos.h"
void gui() {
  while (TRUE) {
    u8 prevTextColor = textColor;

    int totFree = total_used_memory / 1024 / 1024;
    int tot = boot_mmap.total_pages * 4096 / 1024 / 1024;

    const char *title =
        " %2d/%02d/%4d - %02d:%02d:%02d      Used: %4d/%4d Mb"
        "                           P: %3d ";
    disable_int();
    u32 prevPos = getCursorOffset();

    setCursorPos(0, 0);
    setBackgroundColor(BLUE);
    setTextColor(YELLOW);
    read_rtc();
    kprintf(title, rtc_day, rtc_month, rtc_year, rtc_hour, rtc_minute,
            rtc_second, totFree, tot, list_length(&running_queue));

    setCursorOffset(prevPos);
    textColor = prevTextColor;
    enable_int();
    sleep_ms(200);
  }
}
