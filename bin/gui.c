#include "binaries.h"
#include "../drivers/cmos.h"
void gui() {
  while (TRUE) {
    u8 prevTextColor = textColor;

    const char *title =
        " Up: %4ds          "
        "              Desktop 1               %2d/%02d/%4d - %02d:%02d:%02d ";
    disable_int();
    u32 prevPos = getCursorOffset();

    setCursorPos(0, 0);
    setBackgroundColor(BLUE);
    setTextColor(YELLOW);
    read_rtc();
    kprintf(title, get_uptime() / 1000, rtc_day, rtc_month, rtc_year, rtc_hour,
            rtc_minute, rtc_second);

    setCursorOffset(prevPos);
    textColor = prevTextColor;
    enable_int();
    sleep_ms(200);
  }
}
