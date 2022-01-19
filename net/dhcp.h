#ifndef DCHP_H
#define DHCP_H
#include "../cpu/types.h"

typedef struct dchp_option_t {
  u8 type;
  u8 len;
  u8 data;
} __attribute__((packed)) DHCPOption;

typedef struct dhcp_message_t {
  u8 op;
  u8 htype;
  u8 hw_addr_len;
  u8 hops;
  u32 transaction_id;
  u16 secs;
  u16 flags;
  u32 client_ip;
  u32 your_ip;
  u32 server_ip;
  u32 gateway_ip;
  u8 client_mac[6];
  u8 zeroes[202];
  u32 magic_cookie;
  // Options
  DHCPOption option1;
  u8 end_option;
} __attribute__((packed)) DHCPMessage;

void send_dchp_discovery();

#endif