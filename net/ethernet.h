#ifndef ETHERNET_H
#define ETHERNET_H

#include "../cpu/types.h"

#define ETH_ARP 0x0806
#define ETH_IPV4 0x0800

void send_ethernet_packet(byte dst[], u16 eth_type, void *data, u32 len);

#endif