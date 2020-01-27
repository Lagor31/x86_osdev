#ifndef TYPES_H
#define TYPES_H

// We're using sdlib type such as uint32_t and so forth...
#include <stddef.h>
#include <stdint.h>

// Macros to get the top or bottom parts of 32 bits values
#define low_16(address) (uint16_t)((address)&0xFFFF)
#define high_16(address) (uint16_t)(((address) >> 16) & 0xFFFF)

/* Struct which aggregates many registers */
typedef struct {
  uint32_t ds; /* Data segment selector */
  uint32_t edi, esi, ebp, useless, ebx, edx, ecx, eax; /* Pushed by pusha. */
  uint32_t int_no,
      err_code; /* Interrupt number and error code (if applicable) */
  uint32_t eip, cs, eflags, esp, ss; /* Pushed by the processor automatically */
} registers_t;

typedef struct stdDate {
  uint8_t seconds : 6;
  uint16_t minutes : 6;
  uint16_t hours : 5;
  uint16_t days : 15;
} __attribute__((packed)) stdDate_t;

#endif
