
#ifndef KEYBOARD_H
#define KEYBOARD_H

#include "../cpu/types.h"
#include "../lock/lock.h"

#define BACKSPACE 0x0E
#define ENTER 0x1C

void init_keyboard();

typedef struct standard_input {
  Lock *read_lock;
  char *buffer;
  u32 available;
  u32 last;
} Stdin;

extern Stdin stdin;
void init_stdin();
char read_stdin();

#endif