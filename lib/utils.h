#ifndef UTILS_H
#define UTILS_H

#include "../boot/multiboot.h"

void hlt();

void saveMultibootInfo(uint32_t addr, uint32_t magic);
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


#endif