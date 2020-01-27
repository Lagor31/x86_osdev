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
  uint16_t limit;
  uint32_t base;
} __attribute__((packed));

// A struct describing a Task State Segment.
typedef struct tss_entry_struct {
  uint32_t prev_tss;  // The previous TSS - if we used hardware task switching
                      // this would form a linked list.
  uint32_t esp0;  // The stack pointer to load when we change to kernel mode.
  uint32_t ss0;   // The stack segment to load when we change to kernel mode.
  uint32_t esp1;  // everything below here is unusued now..
  uint32_t ss1;
  uint32_t esp2;
  uint32_t ss2;
  uint32_t cr3;
  uint32_t eip;
  uint32_t eflags;
  uint32_t eax;
  uint32_t ecx;
  uint32_t edx;
  uint32_t ebx;
  uint32_t esp;
  uint32_t ebp;
  uint32_t esi;
  uint32_t edi;
  uint32_t es;
  uint32_t cs;
  uint32_t ss;
  uint32_t ds;
  uint32_t fs;
  uint32_t gs;
  uint32_t ldt;
  uint16_t trap;
  uint16_t iomap_base;
} __attribute__((packed)) tss_entry_t;

/* GDT.C */
extern void _gdt_flush();
extern void _tss_flush();

extern void gdt_set_gate(int num, unsigned long base, unsigned long limit,
                         unsigned char access, unsigned char gran);
extern void gdt_install();

#include "mem.h"

static inline struct gdtr *read_gdtr() {
  struct gdtr *gdtr = (struct gdtr *)kmalloc(sizeof(struct gdtr), 1);

  __asm__("sgdt %0" : "=m"(*gdtr));
  //__asm__("mov $31, %%eax\n\r" "mov %%eax, %0" : "=m"(eax));

  return gdtr;
}

extern tss_entry_t tss;

#endif