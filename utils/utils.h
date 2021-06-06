#ifndef UTILS_H
#define UTILS_H


void saveMultibootInfo(uint32_t addr, uint32_t magic);
void printMultibootInfo(struct kmultiboot2info *info, uint8_t onlyMem);
void printKernelMemInfo();
void printInitScreen();
void printHelp();
struct multiboot_tag_module *getModule(struct kmultiboot2info *info);
void printGdt();
uint32_t getRegisterValue(uint8_t reg);
void printModuleInfo(struct multiboot_tag_module *module);
void printUptime();
void getStandardDate(uint32_t millis, stdDate_t *date);
void NMI_enable();
void NMI_disable();
void fakeSysLoadingBar(uint32_t loadingTime);

// RAND_MAX assumed to be 32767
uint32_t rand(void);
void srand(uint32_t seed);
void hlt();
#endif