#include "binaries.h"
#include "../drivers/cmos.h"
void gui() {
  while (TRUE) {
    u8 prevTextColor = textColor;
    uint32_t tot_kern_size_mb = total_kernel_pages * 4096 / 1024 / 1024;

    const char *title =
        " Up: %4ds         "
        "              K:%3d/%3d                %2d/%02d/%4d - %02d:%02d:%02d ";
    // disable_int();
    u32 prevPos = getCursorOffset();

    setCursorPos(0, 0);
    setBackgroundColor(BLUE);
    setTextColor(YELLOW);
    read_rtc();
    kprintf(title, get_uptime() / 1000, total_kused_memory / 1024 / 1024,
            tot_kern_size_mb, rtc_day, rtc_month, rtc_year, rtc_hour,
            rtc_minute, rtc_second);

    setCursorOffset(prevPos);
    textColor = prevTextColor;
    // enable_int();
    sleep_ms(200);
  }
}
