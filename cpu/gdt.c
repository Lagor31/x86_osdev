#include "../cpu/types.h"
#include "../utils/list.h"
#include "../boot/multiboot.h"
#include "../mem/mem.h"
#include "../cpu/gdt.h"

#define GDT_ENTRIES_NUMBER 6
/* Our GDT, with 5 entries, and finally our special GDT pointer */
struct gdt_entry gdt[GDT_ENTRIES_NUMBER];
struct gdtr _gp;
tss_entry_t tss;

/* This will be a function in kernel_entry.asm.  We use this to properly
 * reload the new segment registers */
extern void _gdt_flush();


/* Setup a descriptor in the Global Descriptor Table */
void gdt_set_gate(int num, unsigned long base, unsigned long limit,
                  unsigned char access, unsigned char gran) {
  /* Setup the descriptor base access */
  gdt[num].base_low = (base & 0xFFFF);
  gdt[num].base_middle = (base >> 16) & 0xFF;
  gdt[num].base_high = (base >> 24) & 0xFF;

  /* Setup the descriptor limits */
  gdt[num].limit_low = (limit & 0xFFFF);
  gdt[num].granularity = ((limit >> 16) & 0x0F);

  /* Finally, set up the granularity and access flags */
  gdt[num].granularity |= (gran & 0xF0);
  gdt[num].access = access;
}

/* Should be called by main.  This will setup the special GDT
 * pointer, set up the 6 entries in our GDT, and then finally
 * call gdt_flush() in our assembler file in order to tell
 * the processor where the new GDT is and update the new segment
 * registers. */
void gdt_install() {
  /* Setup the GDT pointer and limit */
  _gp.limit = (sizeof(struct gdt_entry) * GDT_ENTRIES_NUMBER) - 1;
  _gp.base = (unsigned long int)&gdt;

  /* Our NULL descriptor */
  gdt_set_gate(0, 0, 0, 0, 0);

  /* The second entry is our Code Segment.  The base address
   * is 0, the limit is 4 gigabytes, it uses 4 kilobyte
   * granularity, uses 32-bit opcodes, and is a Code Segment
   * descriptor. */
  gdt_set_gate(1, 0, 0xFFFFFFFF, 0x9A, 0xCF);

  /* The third entry is our Data Segment.  It's exactly the
   * same as our code segment, but the descriptor type in
   * this entry's access byte says it's a Data Segment */
  gdt_set_gate(2, 0, 0xFFFFFFFF, 0x92, 0xCF);

  /* Install the user mode segments into the GDT */
  gdt_set_gate(3, 0, 0xFFFFFFFF, 0xFA, 0xCF);
  gdt_set_gate(4, 0, 0xFFFFFFFF, 0xF2, 0xCF);

  tss.ss0 = 0x10;  // Set the kernel stack segment.
  tss.esp0 = (uint32_t) stack_pointer;
  //tss.esp0 = 0x01000;
  gdt_set_gate(5, (uint32_t)&tss, sizeof(tss), 0x89, 0);

  /* Flush our the old GDT / TSS and install the new changes! */
  _gdt_flush();
  _tss_flush();
}
