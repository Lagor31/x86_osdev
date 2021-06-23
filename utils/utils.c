#include "../cpu/types.h"
#include "../boot/multiboot.h"
#include "utils.h"


#include "../utils/list.h"

#include "../mem/mem.h"
#include "../cpu/gdt.h"

#include "../cpu/ports.h"
#include "../cpu/timer.h"
#include "../drivers/screen.h"
#include "../kernel/kernel.h"
#include "../libc/constants.h"


static uint64_t next = 1;
#define RAND_MAX 0xFFFFFF
// RAND_MAX assumed to be 0xFFFFFF

u32 rand() {
  next = next * 1103515245 + 313154;
  return (uint32_t)(next / 65536) % RAND_MAX;
}

void hlt() { __asm__ __volatile__("hlt"); }

void srand(uint32_t seed) { next = seed; }

void saveMultibootInfo(uint32_t addr, uint32_t magic) {
  size_t size = *(uint16_t *)addr;
  kprintf("Bootinfo address in save: 0x%x Magic:  0x%x Size: %d\n", addr, magic,
          size);
  kMultiBootInfo->info = (uint8_t *)boot_alloc(size, 1);
  memcopy((uint8_t *)addr, kMultiBootInfo->info, size);
  kMultiBootInfo->magic = magic;
}

void fakeSysLoadingBar(uint32_t loadingTime) {
  uint32_t waitForEachBar = loadingTime / VGA_COLUMNS;
  for (uint8_t i = 0; i < VGA_COLUMNS; ++i) {
    kprintf(" ");
    syncWait(waitForEachBar);
  }
}

/*
  Only loading first module is supported by now
*/
struct multiboot_tag_module *getModule(struct kmultiboot2info *info) {
  struct multiboot_tag *tag;

  for (tag = (struct multiboot_tag *)(info->info + 8);
       tag->type != MULTIBOOT_TAG_TYPE_END;
       tag = (struct multiboot_tag *)((multiboot_uint8_t *)tag +
                                      ((tag->size + 7) & ~7))) {
    struct multiboot_tag_module *moduleFound;
    switch (tag->type) {
      case MULTIBOOT_TAG_TYPE_MODULE:
        moduleFound = (struct multiboot_tag_module *)tag;
        return moduleFound;
        break;
    }
  }
  return NULL;
}

inline uint32_t getRegisterValue(uint8_t reg) {
  uint32_t regValue = 0;
  switch (reg) {
    case EAX:
      __asm__("mov %%eax, %0" : "=m"(regValue));
      break;
    case EBX:
      __asm__("mov %%ebx, %0" : "=m"(regValue));
      break;
    case ESP:
      __asm__("mov %%esp, %0" : "=m"(regValue));
      break;
    case CR2:
      __asm__(
          "mov %%cr2, %%eax;"
          "mov %%eax, %0"
          : "=m"(regValue));
      break;
    default:
      regValue = -1;
      break;
  }
  return regValue;
}

void printMultibootInfo(struct kmultiboot2info *info, uint8_t onlyMem) {
  struct multiboot_tag *tag;
  unsigned size;

  /*  Am I booted by a Multiboot-compliant boot loader? */
  if (info->magic != MULTIBOOT2_BOOTLOADER_MAGIC) {
    kprintf("Invalid magic number: 0x%x\n", (unsigned)info->magic);
    return;
  }

  /*   if (addr & 7) {
      kprintf("Unaligned mbi: 0x%x\n", addr);
      return;
    }
   */

  uint64_t resMemSize = 0, freeMemSize = 0;
  size = *(unsigned *)info->info;
  if (!onlyMem) kprintf("Announced mbi size 0x%x\n", size);
  for (tag = (struct multiboot_tag *)(info->info + 8);
       tag->type != MULTIBOOT_TAG_TYPE_END;
       tag = (struct multiboot_tag *)((multiboot_uint8_t *)tag +
                                      ((tag->size + 7) & ~7))) {
    if (!onlyMem) kprintf("Tag 0x%x, Size 0x%x\n", tag->type, tag->size);
    switch (tag->type) {
      case MULTIBOOT_TAG_TYPE_MODULE:
        kprintf("Module at 0x%x-0x%x. Command line %s\n",
                ((struct multiboot_tag_module *)tag)->mod_start,
                ((struct multiboot_tag_module *)tag)->mod_end,
                ((struct multiboot_tag_module *)tag)->cmdline);
        break;

      case MULTIBOOT_TAG_TYPE_CMDLINE:
        if (!onlyMem)
          kprintf("Command line = %s\n",
                  ((struct multiboot_tag_string *)tag)->string);
        break;
      case MULTIBOOT_TAG_TYPE_BOOT_LOADER_NAME:
        if (!onlyMem)
          kprintf("Boot loader name = %s\n",
                  ((struct multiboot_tag_string *)tag)->string);
        break;

      case MULTIBOOT_TAG_TYPE_BASIC_MEMINFO:
        if (!onlyMem)
          kprintf("mem_lower = %uKB, mem_upper = %uKB\n",
                  ((struct multiboot_tag_basic_meminfo *)tag)->mem_lower,
                  ((struct multiboot_tag_basic_meminfo *)tag)->mem_upper);
        break;
      case MULTIBOOT_TAG_TYPE_BOOTDEV:
        if (!onlyMem)
          kprintf("Boot device 0x%x,%u,%u\n",
                  ((struct multiboot_tag_bootdev *)tag)->biosdev,
                  ((struct multiboot_tag_bootdev *)tag)->slice,
                  ((struct multiboot_tag_bootdev *)tag)->part);
        break;

      case MULTIBOOT_TAG_TYPE_MMAP: {
        multiboot_memory_map_t *mmap;
        kprintf("System Memory Map:\n");

        for (mmap = ((struct multiboot_tag_mmap *)tag)->entries;
             (multiboot_uint8_t *)mmap < (multiboot_uint8_t *)tag + tag->size;
             mmap = (multiboot_memory_map_t
                         *)((unsigned long)mmap +
                            ((struct multiboot_tag_mmap *)tag)->entry_size)) {
          // if (!onlyMem || (unsigned)mmap->type == 1)
          kprintf(
              " base_addr = 0x%x%x,"
              " length = 0x%x%x, type = 0x%x\n",
              (unsigned)(mmap->addr >> 32), (unsigned)(mmap->addr & 0xffffffff),
              (unsigned)(mmap->len >> 32), (unsigned)(mmap->len & 0xffffffff),
              (unsigned)mmap->type);

          if ((unsigned)mmap->type == 1)
            freeMemSize += (uint64_t)(mmap->len);
          else
            resMemSize += (uint64_t)(mmap->len);
        }
      } break;
    }
  }
  tag = (struct multiboot_tag *)((multiboot_uint8_t *)tag +
                                 ((tag->size + 7) & ~7));

  kprintf("Total memory:      %u MB\n",
          (resMemSize + freeMemSize) / 1024 / 1024);
  kprintf("Reserved memory:   %u KB\n", resMemSize / 1024);
  kprintf("Free memory:       %u MB\n", freeMemSize / 1024 / 1024);
}

void printKernelMemInfo() {
  setTextColor(YELLOW);
  setBackgroundColor(BLUE);
  kprintf("Kernel Memory Info:\n");
  kprintf(" Start address:         0x%x\n", &kimage_start);
  kprintf(" End address:           0x%x\n", &kimage_end);
  kprintf(" Size:                  %d KB\n",
          ((uint32_t)&kimage_end - (uint32_t)&kimage_start) / 1024);
  kprintf(" Entry point:           0x%x\n", &kentry_point);
  kprintf(" Stack pointer:         0x%x\n", stack_pointer);
  kprintf(" Kernel extra memory:   %d KB\n",
          ((uint32_t)free_mem_addr - (uint32_t)&kimage_end) / 1024);
  kprintf(" Kernel total memory:   %d KB\n",
          ((uint32_t)free_mem_addr - (uint32_t)&kimage_start) / 1024);
  kprintf(" Free memory start:     0x%x\n", free_mem_addr);
  kprintf(" Stack size:            %d KB", STACK_SIZE / 1024);
  resetScreenColors();
}

void printInitScreen() {
  setTextColor(WHITE);
  setBackgroundColor(BLUE);
  kprintf(
      "######################################################################"
      "##"
      "########"
      "##                                                                    "
      "  "
      "      ##"
      "##                                                                    "
      "  "
      "      ##"
      "##                                                                    "
      "  "
      "      ##"
      "##                                                                    "
      "  "
      "      ##"
      "##                                GianniOS                            "
      "  "
      "      ##"
      "##                                                                    "
      "  "
      "      ##"
      "##                                                                    "
      "  "
      "      ##"
      "##                                                                    "
      "  "
      "      ##"
      "##                                                                    "
      "  "
      "      ##"
      "##                                                                    "
      "  "
      "      ##"
      "##                               Written by:                          "
      "  "
      "      ##"
      "##                                  Lagor                             "
      "  "
      "      ##"
      "##                                                                    "
      "  "
      "      ##"
      "##                                                                    "
      "  "
      "      ##"
      "##                                                                    "
      "  "
      "      ##"
      "##                  Type help for a list of available commands        "
      "  "
      "      ##"
      "##                                                                    "
      "  "
      "      ##"
      "##                                                                    "
      "  "
      "      ##"
      "##                                                                    "
      "  "
      "      ##"
      "##                                                                    "
      "  "
      "      ##"
      "##                                                                    "
      "  "
      "      ##"
      "######################################################################"
      "##"
      "########");
  resetScreenColors();
}

void printHelp() {
  setTextColor(WHITE);
  setBackgroundColor(GREEN);
  kprintf(
      "clear     -> Clears the screen                                          "
      "       \n");
  kprintf(
      "init      -> Re-prints the initial screen                               "
      "       \n");
  kprintf(
      "gdt       -> Prints the Global Descriptor Table structure               "
      "       \n");
  kprintf(
      "uptime    -> Prints current uptime of the machine                       "
      "       \n");
  kprintf(
      "mods      -> Prints basic info about GRUB2 Modules                      "
      "       \n");
  kprintf(
      "meminfo   -> Shows current kernel memory infos                          "
      "       \n");
  kprintf(
      "mmap      -> Shows available memory block map                           "
      "       \n");
  kprintf(
      "bootinfo  -> Shows multiboot2 info returned by GRUB2                    "
      "       \n");
  kprintf(
      "alloc     -> Allocates a chuck of kernel memory (4KB Aligned)           "
      "       \n");
  kprintf(
      "shutdown  -> Shuts-down the PC                                          "
      "       \n");
  kprintf(
      "reboot    -> Re-boots the PC                                            "
      "       ");
  resetScreenColors();
}

void printGdt() {
  struct gdtr *tGdt = read_gdtr();
  kprintf("GDTR Base: 0x%x  ", tGdt->base);
  kprintf("Limit: %d\n", tGdt->limit);

  struct gdt_entry *gdtEntry = (struct gdt_entry *)tGdt->base;
  for (size_t i = 0; i < (tGdt->limit + 1) / sizeof(struct gdt_entry); ++i) {
    kprintf("[%d]", i);
    kprintf(" Base: 0x%x%x%x   ", gdtEntry->base_high, gdtEntry->base_middle,
            gdtEntry->base_low);
    kprintf(" Limit: 0x%x ", gdtEntry->limit_low);
    kprintf(" Access: 0x%x\n", gdtEntry->access);
    gdtEntry++;
  }
}

stdDate_t *getSystemDate() {
  uint32_t upMillis = getUptime();
  getStandardDate(upMillis, sysDate);
  return sysDate;
}

void getStandardDate(uint32_t millis, stdDate_t *date) {
  date->seconds = millis / MILLIS_IN_A_SECOND;
  kprintf("1\n");
  if (date->seconds >= 60) date->seconds %= 60;
  date->minutes = millis / MILLIS_IN_A_MINUTE;
  kprintf("2\n");
  if (date->minutes >= 60) date->minutes %= 60;
  date->hours = millis / MILLIS_IN_A_HOUR;
  kprintf("3\n");
  if (date->hours >= 24) date->hours %= 24;
  date->days = millis / MILLIS_IN_A_DAY;
}

void printModuleInfo(struct multiboot_tag_module *module) {
  kprintf("Module start:     0x%x\n", module->mod_start);
  kprintf("Module end  :     0x%x\n", module->mod_end);
  kprintf("Module size :     %d", module->mod_end - module->mod_start);
}

void printUptime() {
  /*  stdDate_t *uptime = getSystemDate();
   kprintf("%u days, %02u:%02u:%02u\n", uptime->days, uptime->hours,
           uptime->minutes, uptime->seconds); */
  kprintf("Uptime %d s", getUptime() / 1000);
}

// Enable/Disable non maskable interrupt
void NMI_enable() { outb(0x70, inb(0x70) & 0x7F); }
void NMI_disable() { outb(0x70, inb(0x70) | 0x80); }