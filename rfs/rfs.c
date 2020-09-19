#include "../cpu/types.h"
#include "../utils/list.h"

#include "../drivers/screen.h"
#include "../kernel/kernel.h"
#include "../kernel/mem.h"
#include "../kernel/multiboot.h"

#include "rfs.h"

#include <elf.h>

void loadUserProcess(struct multiboot_tag_module *rfsModule) {
  size_t diskSize = rfsModule->mod_end - rfsModule->mod_start;
  size_t test = 5;
  test++;
  userProcess = boot_alloc(diskSize, 1);
  memcopy((uint8_t *)rfsModule->mod_start + KERNEL_VIRTUAL_ADDRESS_BASE,
          userProcess, diskSize);
  kprintf("User process loaded at 0x%x\n", (uint32_t)userProcess);
}

void printRFSInfo() {
  kprintf("RFS File List (%d):\n", krfsHeader->numberOfFile);

  struct fileTableEntry *curFile = kfileTable;
  for (size_t i = 0; i < krfsHeader->numberOfFile; ++i) {
    kprintf("  File ID=%d; Len=%d; Off=%d\n", curFile->id, curFile->length,
            curFile->offset);
    curFile++;
  }
}
void allocRFS(struct multiboot_tag_module *rfsModule) {
  size_t diskSize = rfsModule->mod_end - rfsModule->mod_start;

  krfsHeader = (struct rfsHeader *)boot_alloc(diskSize, 1);
  memcopy((uint8_t *)rfsModule->mod_start, (uint8_t *)krfsHeader, diskSize);

  if (krfsHeader->magic != RFS_HEADER_MAGIC) {
    setTextColor(WHITE);
    setBackgroundColor(RED);
    kprintf("Loading RFS failed\n");
    resetScreenColors();
    return;
  }

  kfileTable = (struct fileTableEntry *)(krfsHeader + 1);

  kprintf("RFS Disk Loaded in RAM\n");
  kprintf("Disk Size: %d\n", diskSize);
  kprintf("Used size: %d\n", krfsHeader->size);
}
