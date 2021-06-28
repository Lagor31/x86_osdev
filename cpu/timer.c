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
stdDate_t *sysDate;
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

/* static void timer_callback(registers_t *regs) {
  u8 userMode = FALSE;
  if ((regs->cs & 0b11) == 3) userMode = TRUE;

  if (userMode)
    _loadPageDirectory((uint32_t *)PA((uint32_t)kernel_page_directory));


  ++tickCount;
  int prevPos = getCursorOffset();
  setCursorPos(2, 0);
  kprintf("Kernel code! (%d)\n", tickCount);
  setCursorPos(getOffsetRow(prevPos), getOffsetCol(prevPos));

  if (userMode) {
    user_page_directory[0] &= 0xFFFFFFDF;
    user_page_directory[1] &= 0xFFFFFFDF;
    _loadPageDirectory((uint32_t *)PA(
        (uint32_t)&user_page_directory));  // REmove A(ccessed) bit
  }
} */

/*

void initTime(uint32_t freq) {
  //Install the function we just wrote
  register_interrupt_handler(IRQ0, timer_callback);

  // Get the PIT value: hardware clock at 1193180 Hz
  uint32_t divisor = 1193180 / freq;
  u8 low = (u8)(divisor & 0xFF);
  u8 high = (u8)((divisor >> 8) & 0xFF);
  // Send the command
  outb(0x43, 0x36); // Command port
  outb(0x40, low);
  outb(0x40, high);
}

*/
u32 i = 0;
void scheduler_handler(registers_t *regs) {
 
  /*  if (current_proc != NULL)
    kprintf("PID %d - ESP: 0x%x ESP0: 0x%x\n", current_proc->pid,
            getRegisterValue(ESP), tss.esp0);  */
  // I was in user mod
  /*  u8 userMode = FALSE;
   if ((regs->cs & 0b11) == 3) userMode = TRUE;
*/

  ++tickCount;
  current_proc->running_ticks++;

  if (current_proc != NULL && current_proc->sched_count-- <= 0)
    // reschedule
    next_proc = (Proc *)do_schedule();
  else
    next_proc = current_proc;

  srand(tickCount);

  if (next_proc != NULL && next_proc != current_proc && current_proc != NULL) {
    //_loadPageDirectory((uint32_t *)PA((uint32_t)&kernel_page_directory));

    outb(0x70, 0x0C); // select register C
    inb(0x71);        // just throw away contents
    if (regs->int_no >= IRQ8) {
      outb(0xA0, 0x20); /* slave */
      outb(0x20, 0x20);
    }
    _switch_to_task(next_proc);
    return;
  }

  if (current_proc != NULL) {
    current_proc->regs.eip = regs->eip;
    current_proc->regs.esp = regs->esp;
    current_proc->esp0 = tss.esp0;
  }

  //Enable interrupts if no context switch was necessary
  regs->eflags |= 0x200;

  /*
    Need to reset the register C otherwise no more RTC interrutps will be sent
  */
  outb(0x70, 0x0C); // select register C
  inb(0x71);        // just throw away contents
}

void init_scheduler_timer() {
  setTimerPhase(RTC_PHASE);
  register_interrupt_handler(IRQ8, scheduler_handler);
  tss.esp0 = getRegisterValue(ESP);
  tss.ss0 = 0x10;
}

void setTimerPhase(u16 rate) {
  // NMI_disable();

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
  // NMI_enable();
}