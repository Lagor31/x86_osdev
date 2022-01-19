#ifndef IPV4_H
#define IPV4_H

#include "../cpu/types.h"

typedef struct ipv4_header_t {
  u8 ihl : 4;
  u8 version : 4;

  u8 dscp : 6;
  u8 ecn : 2;

  u16 total_length;
  u16 identification;

  u8 flags : 3;
  u16 fragment_offset : 13;

  u8 ttl;
  u8 protocol;
  u16 header_checksum;
  u8 source[4];
  u8 dest[4];
} __attribute__((packed)) IPv4Header;

IPv4Header* create_ipv4_header(byte src_ip[], byte dest_ip[], u32 len);
void send_ip_packet(IPv4Header* ip);

#endif