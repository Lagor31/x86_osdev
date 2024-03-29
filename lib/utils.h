#ifndef UTILS_H
#define UTILS_H

#include "../boot/multiboot.h"


/* Sometimes we want to keep parameters to a function for later use
 * and this is a solution to avoid the 'unused parameter' compiler warning */
#define UNUSED(x) (void)(x)


extern void _setreg(u32 reg, u32 value);

void hlt();

void save_multiboot2_info(uint32_t addr, uint32_t magic);
void printMultibootInfo(KMultiBoot2Info *info, uint8_t onlyMem);
void printKernelMemInfo();
void printInitScreen();
void printHelp();
struct multiboot_tag_module *getModule(KMultiBoot2Info *info);
void printGdt();
uint32_t getRegisterValue(uint8_t reg);
void printModuleInfo(struct multiboot_tag_module *module);
void printUptime();
void getStandardDate(uint32_t millis, stdDate_t *date);
void NMI_enable();
void NMI_disable();
void fakeSysLoadingBar(uint32_t loadingTime);

// RAND_MAX assumed to be 32767
u32 rand(void);
void srand(u32 seed);
bool ints_enabled();

static inline u32 native_save_fl(void) {
  u32 flags;

  /*
   * "=rm" is safe here, because "pop" adjusts the stack before
   * it evaluates its effective address -- this is part of the
   * documented behavior of the "pop" instruction.
   */
  asm volatile(
      "# __raw_save_flags\n\t"
      "pushf ; pop %0"
      : "=rm"(flags)
      : /* no input */
      : "memory");

  return flags;
}

#endif