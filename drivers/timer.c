#include "timer.h"

#include "../cpu/gdt.h"
#include "../drivers/screen.h"
#include "../kernel/kernel.h"
#include "../kernel/scheduler.h"
#include "../lib/constants.h"
#include "../mem/mem.h"
#include "../mem/paging.h"
#include "../proc/thread.h"
#include "../lib/list.h"
#include "../lib/utils.h"
#include "../cpu/isr.h"
#include "../cpu/ports.h"
#include "cmos.h"
#include "../mem/buddy.h"


unsigned long long last_timer_int = 0;
/*
   Even at the highest frequency (4096 ticks/s, given by RTC_PHASE = 4) it
   would take 139.4  MILLENNIA to wrap around, i think we're safe...
*/
u64 tick_count = 0;
unsigned long long cycles_passed = 0;

/* Returns the number of milliseconds since RTC setup */
u32 get_uptime() { return (u32)ticks_to_millis(tick_count); }

inline u64 millis_to_ticks(u32 millis) { return millis * RTC_FREQ / 1000; }

inline u32 ticks_to_millis(u64 tickCount) {
  return 1000 * tickCount / RTC_FREQ;
  // return (u32)((tickCount * ((double)1 / (double)RTC_FREQ)) * 1000);
};

void scheduler_handler(registers_t *regs) {
  ++tick_count;
  Thread *next_thread;
  
  
  //u32 alloc_ptr_sched[ALLOC_NUM];

  /* for (u32 i = 0; i < ALLOC_NUM; ++i) {
    u32 r = rand() % (8192);
    // kprintf("%d) Size: %d -> ", i, r);
    alloc_ptr_sched[i] = (u32)fmalloc(r);
    // ffree((void *) alloc_ptr[i]);
    // free_buddy_new(alloc_b[i], &fast_buddy_new);
    // print_buddy_new(alloc_b[i]);
  } */

  // kprintf("\n");
  // Wake up all processes that no longer need to sleep on timers
  wake_up_timers();

  /* for (u32 k = 0; k < ALLOC_NUM; ++k) {
    //    kprintf("%d) Freeing\n", k);
    // if (k % 3 == 0) continue;
    // print_buddy_new(alloc_b[k]);
    ffree((void *)alloc_ptr_sched[k]);
  } */
  // kprintf("\n");

  if (current_thread != NULL && current_thread->timeslice > 0) goto no_resched;

  // reschedule
  next_thread = (Thread *)pick_next_thread();

  if (current_thread == next_thread) goto no_resched;

  if (next_thread != NULL) {
    outb(0x70, 0x0C);  // select register C
    inb(0x71);         // just throw away contents
    if (regs->int_no >= IRQ8) {
      outb(0xA0, 0x20); /* slave */
      outb(0x20, 0x20);
    }
    if (next_thread->timeslice > 0) next_thread->timeslice--;
    next_thread->last_activation = rdtscl();
    if (current_thread != NULL && current_thread != idle_thread)
      current_thread->runtime +=
          (next_thread->last_activation - current_thread->last_activation);
    _switch_to_thread(next_thread);

    outb(0x70, 0x0C);  // select register C
    inb(0x71);         // just throw away contents
    return;
  }

no_resched:
  // current_thread->runtime++;
  unsigned long long now = rdtscl();
  if (current_thread != idle_thread)
    current_thread->runtime += (now - current_thread->last_activation);
  current_thread->last_activation = now;
  if (current_thread->timeslice > 0) current_thread->timeslice--;
  /*
    Need to reset the register C otherwise no more RTC interrutps will be sent
  */
  handle_signals(current_thread);

  outb(0x70, 0x0C);  // select register C
  inb(0x71);         // just throw away contents
}

void init_scheduler_timer() {
  // asm  volatile("cli");
  set_timer_phase(RTC_PHASE);
  register_interrupt_handler(IRQ8, scheduler_handler);
  tss.esp0 = getRegisterValue(ESP);
  tss.ss0 = 0x10;
  // asm  volatile("sti");
}

void set_timer_phase(u16 rate) {
  NMI_disable();
  rate &= 0x0F;  // rate must be above 2 and not over 15
  /* Setting up rate */
  outb(0x70, 0x8A);                 // set index to register A, disable NMI
  char prev = inb(0x71);            // get initial value of register A
  outb(0x70, 0x8A);                 // reset index to A
  outb(0x71, (prev & 0xF0) | rate); /* write only our rate to A. Note, rate is
                                       the bottom 4 bits. */
  /* Enabling RTC */
  outb(0x70, 0x8B);  // select register B, and disable NMI
  prev = inb(0x71);  // read the current value of register B
  outb(0x70, 0x8B);  /* set the index again (a read will reset the index to
                        register D) */
  outb(0x71, prev | 0x40); /* write the previous value ORed with 0x40. This
                            turns on bit 6 of register B */
  NMI_enable();
}

unsigned long long rdtscl() {
  unsigned int lo, hi;
  __asm__ __volatile__("rdtsc" : "=a"(lo), "=d"(hi));
  return ((unsigned long long)lo) | (((unsigned long long)hi) << 32);
}