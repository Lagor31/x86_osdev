#ifndef TIMER_H
#define TIMER_H

#include "../cpu/types.h"
#include "../cpu/gdt.h"

#define RTC_PHASE 4
#define RTC_FREQ (32768 >> (RTC_PHASE - 1))
#define MILLIS_IN_A_YEAR 3.154E10
#define MILLIS_IN_A_DAY 8.64E7
#define MILLIS_IN_A_HOUR 3.6E6
#define MILLIS_IN_A_MINUTE 6E4
#define MILLIS_IN_A_SECOND 1000


extern uint64_t tick_count;
extern tss_entry_t tss;

void scheduler_handler(registers_t* regs);
void init_scheduler_timer();
void initTime(uint32_t freq);
void set_timer_phase(uint16_t hz);
uint64_t millis_to_ticks(uint32_t millis);
uint32_t ticks_to_millis(uint64_t ticks);
uint32_t get_uptime();

#endif