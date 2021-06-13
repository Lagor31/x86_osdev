#ifndef TIMER_H
#define TIMER_H
#include "gdt.h"

#define RTC_PHASE 15
#define RTC_FREQ (32768 >> (RTC_PHASE - 1))
#define MILLIS_IN_A_YEAR 3.154E10
#define MILLIS_IN_A_DAY 8.64E7
#define MILLIS_IN_A_HOUR 3.6E6
#define MILLIS_IN_A_MINUTE 6E4
#define MILLIS_IN_A_SECOND 1000


extern uint64_t tickCount;
extern stdDate_t* sysDate;
extern tss_entry_t tss;

void timerHandler(registers_t* regs);
void init_scheduler_timer();
void initTime(uint32_t freq);
void setTimerPhase(uint16_t hz);
uint64_t millisToTicks(uint32_t millis);
uint32_t ticksToMillis(uint64_t ticks);
void syncWait(uint32_t);
uint32_t getUptime();

#endif