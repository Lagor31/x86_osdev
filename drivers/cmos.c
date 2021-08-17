#include "../cpu/ports.h"
#include "../cpu/types.h"

#include "screen.h"

#define CURRENT_YEAR 2021  // Change this each year!
#define TIME_ZONE_DIFF 2

int century_register = 0x00;  // Set by ACPI table parsing code if possible

unsigned char rtc_second;
unsigned char rtc_minute;
unsigned char rtc_hour;
unsigned char rtc_day;
unsigned char rtc_month;
unsigned int rtc_year;

enum { cmos_address = 0x70, cmos_data = 0x71 };
void print_date() {
  kprintf("%d/%d/%d - %d:%d:%d\n", rtc_day, rtc_month, rtc_year, rtc_hour, rtc_minute, rtc_second);
}

int get_update_in_progress_flag() {
  outb(cmos_address, 0x0A);
  return (inb(cmos_data) & 0x80);
}

unsigned char get_RTC_register(int reg) {
  outb(cmos_address, reg);
  return inb(cmos_data);
}

void read_rtc() {
  unsigned char century;
  unsigned char last_second;
  unsigned char last_minute;
  unsigned char last_hour;
  unsigned char last_day;
  unsigned char last_month;
  unsigned char last_year;
  unsigned char last_century;
  unsigned char registerB;

  // Note: This uses the "read registers until you get the same values twice in
  // a row" technique
  //       to avoid getting dodgy/inconsistent values due to RTC updates

  while (get_update_in_progress_flag())
    ;  // Make sure an update isn't in progress
  rtc_second = get_RTC_register(0x00);
  rtc_minute = get_RTC_register(0x02);
  rtc_hour = get_RTC_register(0x04);
  rtc_day = get_RTC_register(0x07);
  rtc_month = get_RTC_register(0x08);
  rtc_year = get_RTC_register(0x09);
  if (century_register != 0) {
    century = get_RTC_register(century_register);
  }

  do {
    last_second = rtc_second;
    last_minute = rtc_minute;
    last_hour = rtc_hour;
    last_day = rtc_day;
    last_month = rtc_month;
    last_year = rtc_year;
    last_century = century;

    while (get_update_in_progress_flag())
      ;  // Make sure an update isn't in progress
    rtc_second = get_RTC_register(0x00);
    rtc_minute = get_RTC_register(0x02);
    rtc_hour = get_RTC_register(0x04);
    rtc_day = get_RTC_register(0x07);
    rtc_month = get_RTC_register(0x08);
    rtc_year = get_RTC_register(0x09);
    if (century_register != 0) {
      century = get_RTC_register(century_register);
    }
  } while ((last_second != rtc_second) || (last_minute != rtc_minute) ||
           (last_hour != rtc_hour) || (last_day != rtc_day) || (last_month != rtc_month) ||
           (last_year != rtc_year) || (last_century != century));

  registerB = get_RTC_register(0x0B);

  // Convert BCD to binary values if necessary

  if (!(registerB & 0x04)) {
    rtc_second = (rtc_second & 0x0F) + ((rtc_second / 16) * 10);
    rtc_minute = (rtc_minute & 0x0F) + ((rtc_minute / 16) * 10);
    rtc_hour = ((rtc_hour & 0x0F) + (((rtc_hour & 0x70) / 16) * 10)) | (rtc_hour & 0x80);
    rtc_day = (rtc_day & 0x0F) + ((rtc_day / 16) * 10);
    rtc_month = (rtc_month & 0x0F) + ((rtc_month / 16) * 10);
    rtc_year = (rtc_year & 0x0F) + ((rtc_year / 16) * 10);
    if (century_register != 0) {
      century = (century & 0x0F) + ((century / 16) * 10);
    }
  }

  // Convert 12 hour clock to 24 hour clock if necessary

  if (!(registerB & 0x02) && (rtc_hour & 0x80)) {
    rtc_hour = ((rtc_hour & 0x7F) + 12) % 24;
  }

  // Calculate the full (4-digit) year

  if (century_register != 0) {
    rtc_year += century * 100;
  } else {
    rtc_year += (CURRENT_YEAR / 100) * 100;
    if (rtc_year < CURRENT_YEAR) rtc_year += 100;
  }

  rtc_hour += TIME_ZONE_DIFF;
}