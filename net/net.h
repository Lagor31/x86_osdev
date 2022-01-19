#ifndef NET_H
#define NET_H

#include "../cpu/types.h"

void init_networking();
void print_ethernet_packet(void *);

u16 switch_endian16(u16 nb);
u32 switch_endian32(u32 nb);
bool is_equal_macs(byte *mac1, byte *mac2);

#endif