#ifndef ARP_H
#define ARP_H

#include "../cpu/types.h"
#include "../mem/mem.h"
#include "../net/ethernet.h"

#define ARP_ETH 0x0001
#define ARP_IPV4 0x0800

#define ARP_ETH_SIZE 6
#define ARP_IPV4_SIZE 4

#define ARP_REQ 1
#define ARP_REPLY 2

#define ARP_REQ_LEN 28

// Sender mac, sender ip
// Target mac, target ip

typedef struct arp_packet_t {
  u16 type;
  u16 protocol;
  u8 hwsize;
  u8 psize;
  u16 opcode;
  /* data */
} __attribute__((packed)) ArpPacket;

void send_arp_request(byte ip[]);
#endif