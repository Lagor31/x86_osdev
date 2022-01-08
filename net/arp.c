#include "arp.h"
#include "../net/net.h"

void send_arp_request(byte ip[]) {
  ArpPacket *arp_req = kmalloc(ARP_REQ_LEN);
  arp_req->type = switch_endian16(ARP_ETH);
  arp_req->protocol = switch_endian16(ARP_IPV4);
  arp_req->hwsize = ARP_ETH_SIZE;
  arp_req->psize = ARP_IPV4_SIZE;
  arp_req->opcode = switch_endian16(ARP_REQ);
  byte brd[] = {0xff, 0xff, 0xff, 0xff, 0xff, 0xff};
  send_ethernet_packet(brd, ETH_ARP, arp_req, ARP_REQ_LEN);
}