#ifndef PAGE_H
#define PAGE_H

#include "../lib/list.h"

#define PAGE_SIZE 4096
#define PD_SIZE 1024
#define PT_SIZE 1024

typedef struct Page {
  // 8 Bits for various flags
  u8 flags;
} Page;

typedef uint32_t Pte;

static inline void setReadWrite(Pte *x) { *x |= (1L << 1); }
static inline void setPfn(Pte *x, uint32_t pfn) {
  *x &= 0xFFF;
  *x |= (pfn << 12);
}
static inline void setPresent(Pte *x) { *x |= (1L << 0); }
static inline void setUsermode(Pte *x) { *x |= (1L << 2); }
static inline uint8_t isPresent(Pte *x) { return *x & 1; }
static inline bool isUsermode(Pte *x) { return *x & 4; }

#endif