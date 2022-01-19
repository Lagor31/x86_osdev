#ifndef UDP_H
#define UDP_H
#include "../cpu/types.h"

typedef struct udp_packet_t {
  u16 src_port;
  u16 dst_port;
  u16 len;
  u16 checksum;
} __attribute__((packed)) UDP;


UDP* create_udp_packet(u16 src_port, u16 dest_port, u16 len, u16 checksum);
#endif