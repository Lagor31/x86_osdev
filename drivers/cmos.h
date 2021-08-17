#ifndef CMOS_H
#define CMOS_H

extern unsigned char rtc_second;
extern unsigned char rtc_minute;
extern unsigned char rtc_hour;
extern unsigned char rtc_day;
extern unsigned char rtc_month;
extern unsigned int rtc_year;

void print_date();
#endif