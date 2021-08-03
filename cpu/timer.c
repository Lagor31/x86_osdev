#include "types.h"

#include "../utils/list.h"

#include "../kernel/kernel.h"
#include "../mem/mem.h"
#include "../mem/paging.h"

#include "../cpu/gdt.h"

#include "../drivers/screen.h"
#include "../libc/constants.h"
#include "../libc/functions.h"
#include "../proc/proc.h"
#include "../utils/utils.h"

#include "isr.h"
#include "ports.h"

#include "timer.h"

/*
   Even at the highest frequency (4096 ticks/s, given by RTC_PHASE = 4) it
   would take 139.4  MILLENNIA to wrap around, i think we're safe...
*/
u64 tickCount = 0;
Proc *next_proc;

/*Halts the cpu and stays idle until the timer has expired */
void syncWait(u32 millis) {
  u64 startTicks = tickCount;
  u64 totalWaitingTicks = millisToTicks(millis);
  while (tickCount < startTicks + totalWaitingTicks) {
    __asm__("hlt");
  }
}

/* Returns the number of milliseconds since RTC setup */
u32 getUptime() { return (u32)ticksToMillis(tickCount); }

inline u64 millisToTicks(u32 millis) { return millis * RTC_FREQ / 1000; }

inline u32 ticksToMillis(u64 tickCount) {
  return (u32)((tickCount * ((double)1 / (double)RTC_FREQ)) * 1000);
};

void scheduler_handler(registers_t *regs) {
  ++tickCount;
  if (current_proc != NULL && current_proc != kwork_thread && current_proc->sched_count > 0)
    current_proc->sched_count--;
  current_proc->runtime++;

  if (work_queue_lock->state == LOCK_LOCKED) {
    goto done_sched;
  }

  // Wake up all processes that no longer need to sleep on locks or timers
  wake_up_all();
  // reschedule
  next_proc = (Proc *)do_schedule();

  if (next_proc != NULL && next_proc != current_proc) {
    outb(0x70, 0x0C); // select register C
    inb(0x71);        // just throw away contents
    if (regs->int_no >= IRQ8) {
      outb(0xA0, 0x20); /* slave */
      outb(0x20, 0x20);
    }
    _switch_to_task(next_proc);
  }
done_sched:
  // Enable interrupts if no context switch was necessary
  regs->eflags |= 0x200;
  /*
    Need to reset the register C otherwise no more RTC interrutps will be sent
  */
  outb(0x70, 0x0C); // select register C
  inb(0x71);        // just throw away contents
}

void init_scheduler_timer() {
  // asm  volatile("cli");
  setTimerPhase(RTC_PHASE);
  register_interrupt_handler(IRQ8, scheduler_handler);
  tss.esp0 = getRegisterValue(ESP);
  tss.ss0 = 0x10;
  // asm  volatile("sti");
}

void setTimerPhase(u16 rate) {
  NMI_disable();
  rate &= 0x0F; // rate must be above 2 and not over 15
  /* Setting up rate */
  outb(0x70, 0x8A);                 // set index to register A, disable NMI
  char prev = inb(0x71);            // get initial value of register A
  outb(0x70, 0x8A);                 // reset index to A
  outb(0x71, (prev & 0xF0) | rate); /* write only our rate to A. Note, rate is
                                       the bottom 4 bits. */
  /* Enabling RTC */
  outb(0x70, 0x8B); // select register B, and disable NMI
  prev = inb(0x71); // read the current value of register B
  outb(0x70, 0x8B); /* set the index again (a read will reset the index to
                       register D) */
  outb(0x71, prev | 0x40); /* write the previous value ORed with 0x40. This
                            turns on bit 6 of register B */
  NMI_enable();
}