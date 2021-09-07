
#ifndef KEYBOARD_H
#define KEYBOARD_H

#include "../cpu/types.h"
#include "../lock/lock.h"

#define BACKSPACE 0x0E
#define ENTER 0x1C
#define CTRL 29

void init_keyboard();
char read_stdin();

extern char keyboard_buffer[1024];
extern u32 key_buf_avail;
extern u32 key_buf_last;
extern const char sc_ascii[];

#endif