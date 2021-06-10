#ifndef TYPES_H
#define TYPES_H

// We're using sdlib type such as uint32_t and so forth...
#include <stddef.h>
#include <stdint.h>

typedef uint8_t u8;
typedef u8 byte;
typedef u8 bool;

typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;

// Macros to get the top or bottom parts of 32 bits values
#define low_16(address) (uint16_t)((address)&0xFFFF)
#define high_16(address) (uint16_t)(((address) >> 16) & 0xFFFF)

/* Struct which aggregates many registers */
typedef struct {
  u32 ds; /* Data segment selector */
  u32 edi, esi, ebp, useless, ebx, edx, ecx, eax; /* Pushed by pusha. */
  u32 int_no,
      err_code; /* Interrupt number and error code (if applicable) */
  u32 eip, cs, eflags, esp, ss; /* Pushed by the processor automatically */
} registers_t;

typedef struct stdDate {
  uint8_t seconds : 6;
  uint16_t minutes : 6;
  uint16_t hours : 5;
  uint16_t days : 15;
} __attribute__((packed)) stdDate_t;

#endif
