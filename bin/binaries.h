#ifndef BINARIES_H
#define BINARIES_H


#include "../kernel/kernel.h"
#include "../cpu/types.h"
#include "../drivers/screen.h"
#include "../mem/mem.h"
#include "../drivers/keyboard.h"
#include "../lib/list.h"
#include "../proc/thread.h"


extern void top_bar();
extern void k_simple_proc();
extern void shell();
extern void u_simple_proc();
extern void top();

#endif