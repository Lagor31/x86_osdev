#include "ports.h"
#include "../cpu/types.h"
/**
 * Read a byte from the specified port
 */
inline unsigned char inb(unsigned short port) {
  unsigned char result;
  /* Inline assembler syntax
   * !! Notice how the source and destination registers are switched from NASM
   * !!
   *
   * '"=a" (result)'; set '=' the C variable '(result)' to the value of register
   * e'a'x
   * '"d" (port)': map the C variable '(port)' into e'd'x register
   *
   * Inputs and outputs are separated by colons
   */
  __asm__("in %%dx, %%al" : "=a"(result) : "d"(port));
  return result;
}

inline void outb(unsigned short port, unsigned char data) {
  /* Notice how here both registers are mapped to C variables and
   * nothing is returned, thus, no equals '=' in the asm syntax
   * However we see a comma since there are two variables in the input area
   * and none in the 'return' area
   */
  __asm__("out %%al, %%dx" : : "a"(data), "d"(port));
}

unsigned short inw(unsigned short port) {
  unsigned short result;
  __asm__("in %%dx, %%ax" : "=a"(result) : "d"(port));
  return result;
}

void outw(unsigned short port, unsigned short data) {
  __asm__("out %%ax, %%dx" : : "a"(data), "d"(port));
}

void outdw(unsigned short port, u32 data) {
  __asm__("out %%eax, %%dx" : : "a"(data), "d"(port));
}

u32 indw(unsigned short port) {
  u32 result;
  __asm__("in %%dx, %%eax" : "=a"(result) : "d"(port));
  return result;
}
