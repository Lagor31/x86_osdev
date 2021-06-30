#ifndef LOCK_H
#define LOCK_H

#include "../cpu/types.h"

extern void _spin_lock(u32* state);
extern void _free_lock(u32* state);

#endif