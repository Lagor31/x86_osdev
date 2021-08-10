
#ifndef KEYBOARD_H
#define KEYBOARD_H

#include "../cpu/types.h"
#include "../lock/lock.h"

#define BACKSPACE 0x0E
#define ENTER 0x1C
#define CTRL 29

void init_keyboard();
char read_stdin();

#endif