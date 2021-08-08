
#ifndef KEYBOARD_H
#define KEYBOARD_H

#include "../cpu/types.h"
#include "../lock/lock.h"
#include "../kernel/files.h"

#define BACKSPACE 0x0E
#define ENTER 0x1C

void init_keyboard();
byte read_stdin();

#endif