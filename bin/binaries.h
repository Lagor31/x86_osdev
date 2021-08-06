#ifndef BINARIES_H
#define BINARIES_H


#include "../kernel/kernel.h"
#include "../cpu/types.h"
#include "../drivers/screen.h"
#include "../mem/mem.h"
#include "../drivers/keyboard.h"
#include "../lib/list.h"
#include "../proc/thread.h"
#include "../lib/utils.h"
#include "../lib/strings.h"
#include "../lib/shutdown.h"
#include "../kernel/scheduler.h"
#include "../kernel/syscall.h"

extern void top_bar();
extern void k_simple_proc();
extern void shell();
extern void u_simple_proc();
extern void top();
extern void itaFlag();

#endif