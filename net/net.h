#ifndef NET_H
#define NET_H

#include "../cpu/types.h"

void init_networking();
void print_ethernet_packet(void *);

u32 switch_endian16(u16 nb);

u32 switch_endian32(u32 nb);

#endif