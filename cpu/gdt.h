#ifndef GDT_H
#define GDT_H

/* Defines a GDT entry.  We say packed, because it prevents the
 * compiler from doing things that it thinks is best, i.e.
 * optimization, etc. */
struct gdt_entry {
  unsigned short limit_low;
  unsigned short base_low;
  unsigned char base_middle;
  unsigned char access;
  unsigned char granularity;
  unsigned char base_high;
} __attribute__((packed));

/* Special pointer which includes the limit: The max bytes
 * taken up by the GDT, minus 1.  Again, this NEEDS to be
 * packed */
struct gdtr {
  u16 limit;
  u32 base;
} __attribute__((packed));

// A struct describing a Task State Segment.
typedef struct tss_entry_struct {
  u32 prev_tss;  // The previous TSS - if we used hardware task switching
                      // this would form a linked list.
  u32 esp0;  // The stack pointer to load when we change to kernel mode.
  u32 ss0;   // The stack segment to load when we change to kernel mode.
  u32 esp1;  // everything below here is unusued now..
  u32 ss1;
  u32 esp2;
  u32 ss2;
  u32 cr3;
  u32 eip;
  u32 eflags;
  u32 eax;
  u32 ecx;
  u32 edx;
  u32 ebx;
  u32 esp;
  u32 ebp;
  u32 esi;
  u32 edi;
  u32 es;
  u32 cs;
  u32 ss;
  u32 ds;
  u32 fs;
  u32 gs;
  u32 ldt;
  u16 trap;
  u16 iomap_base;
} __attribute__((packed)) tss_entry_t;

/* GDT.C */
extern void _gdt_flush();
extern void _tss_flush();

extern void gdt_set_gate(int num, unsigned long base, unsigned long limit,
                         unsigned char access, unsigned char gran);
extern void gdt_install();

#include "../mem/mem.h"

static inline struct gdtr *read_gdtr() {
  struct gdtr *gdtr = (struct gdtr *)boot_alloc(sizeof(struct gdtr), 1);

  __asm__("sgdt %0" : "=m"(*gdtr));
  //__asm__("mov $31, %%eax\n\r" "mov %%eax, %0" : "=m"(eax));

  return gdtr;
}

extern tss_entry_t tss;

#endif