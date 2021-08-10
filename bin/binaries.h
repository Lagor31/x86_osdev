#ifndef BINARIES_H
#define BINARIES_H

#include "../proc/thread.h"

#include "../kernel/kernel.h"
#include "../cpu/types.h"
#include "../drivers/screen.h"
#include "../mem/mem.h"
#include "../drivers/keyboard.h"
#include "../lib/list.h"
#include "../lib/utils.h"
#include "../lib/strings.h"
#include "../lib/shutdown.h"
#include "../kernel/scheduler.h"
#include "../kernel/syscall.h"
#include "../users/user.h"

extern void top_bar();
extern void k_simple_proc();
extern void shell();
extern void u_simple_proc();
extern void top();
extern void itaFlag();
extern void init();
extern void ps();
extern void login();
extern void print_single_thread(Thread *p);
extern void gui();
extern void k_child_proc();

#endif