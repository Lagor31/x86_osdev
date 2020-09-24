#include "types.h"

#include "../utils/list.h"

#include "../kernel/kernel.h"
#include "../mem/mem.h"
#include "../mem/paging.h"

#include "../cpu/gdt.h"

#include "../drivers/screen.h"
#include "../libc/constants.h"
#include "../libc/functions.h"
#include "../utils/utils.h"
#include "isr.h"
#include "ports.h"

#include "timer.h"

/*
   Even at the highest frequency (4096 ticks/s, given by RTC_PHASE = 4) it
   would take 139.4  MILLENNIA to wrap around, i think we're safe...
*/
uint64_t tickCount = 0;
stdDate_t *sysDate;

/*Halts the cpu and stays idle until the timer has expired */
void syncWait(uint32_t millis) {
  uint64_t startTicks = tickCount;
  uint64_t totalWaitingTicks = millisToTicks(millis);
  while (tickCount < startTicks + totalWaitingTicks) {
    __asm__("hlt");
  }
}
/* Returns the number of milliseconds since RTC setup */
uint32_t getUptime() { return (uint32_t)ticksToMillis(tickCount); }

inline uint64_t millisToTicks(uint32_t millis) {
  return millis / (((double)1 / (double)RTC_FREQ) * 1000);
}

inline uint32_t ticksToMillis(uint64_t tickCount) {
  return (uint32_t)((tickCount * ((double)1 / (double)RTC_FREQ)) * 1000);
};

static void timer_callback(registers_t *regs) {
  uint8_t userMode = FALSE;
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
}

void initTime(uint32_t freq) {
  /* Install the function we just wrote */
  register_interrupt_handler(IRQ0, timer_callback);

  /* Get the PIT value: hardware clock at 1193180 Hz */
  uint32_t divisor = 1193180 / freq;
  uint8_t low = (uint8_t)(divisor & 0xFF);
  uint8_t high = (uint8_t)((divisor >> 8) & 0xFF);
  /* Send the command */
  outb(0x43, 0x36); /* Command port */
  outb(0x40, low);
  outb(0x40, high);
}

void timerHandler(registers_t *regs) {
  /*
    Need to reset the register C otherwise no more RTC interrutps will be sent
   */

  // I was in user mode
  uint8_t userMode = FALSE;
  if ((regs->cs & 0b11) == 3) userMode = TRUE;

  if (userMode)
    _loadPageDirectory((uint32_t *)PA((uint32_t)kernel_page_directory));

  ++tickCount;
  
  UNUSED(regs);
  regs->eflags |= 0x200;

  tss.ss0 = 0x10;
  tss.esp0 = getRegisterValue(ESP);
  outb(0x70, 0x0C);  // select register C
  inb(0x71);         // just throw away contents

  if (userMode)
    _loadPageDirectory((uint32_t *)PA((uint32_t)&user_page_directory));
}

void initTimer() {
  sysDate = (stdDate_t *)boot_alloc(sizeof(stdDate_t), 0);
  setTimerPhase(RTC_PHASE);
  register_interrupt_handler(IRQ8, timerHandler);
}

void setTimerPhase(uint16_t rate) {
  // NMI_disable();

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
  // NMI_enable();
}