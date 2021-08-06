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

/*
   Even at the highest frequency (4096 ticks/s, given by RTC_PHASE = 4) it
   would take 139.4  MILLENNIA to wrap around, i think we're safe...
*/
u64 tick_count = 0;

/* Returns the number of milliseconds since RTC setup */
u32 get_uptime() { return (u32)ticks_to_millis(tick_count); }

inline u64 millis_to_ticks(u32 millis) { return millis * RTC_FREQ / 1000; }

inline u32 ticks_to_millis(u64 tickCount) {
  return 1000 * tickCount / RTC_FREQ;
  // return (u32)((tickCount * ((double)1 / (double)RTC_FREQ)) * 1000);
};

void scheduler_handler(registers_t *regs) {
  ++tick_count;
  if (current_thread != NULL) {
    current_thread->runtime++;
    if (current_thread != kwork_thread && current_thread->sched_count > 0)
      current_thread->sched_count--;
  }

  if (work_queue_lock->state == LOCK_LOCKED) goto done_sched;

  // Wake up all processes that no longer need to sleep on locks or timers
  wake_up_all();
  // reschedule
  Thread *next_thread = (Thread *)do_schedule();

  if (next_thread != NULL && next_thread != current_thread) {
    outb(0x70, 0x0C);  // select register C
    inb(0x71);         // just throw away contents
    if (regs->int_no >= IRQ8) {
      outb(0xA0, 0x20); /* slave */
      outb(0x20, 0x20);
    }
    _switch_to_thread(next_thread);
  }
done_sched:
  // Enable interrupts if no context switch was necessary
  //regs->eflags |= 0x200;
  /*
    Need to reset the register C otherwise no more RTC interrutps will be sent
  */
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