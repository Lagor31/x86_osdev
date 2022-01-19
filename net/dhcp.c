#include "dhcp.h"
#include "../net/net.h"
#include "../mem/mem.h"
#include "ethernet.h"
#include "../drivers/rtl8139.h"
#include "udp.h"
#include "ipv4.h"
#include "../lib/utils.h"

DHCPMessage *create_dchp_discovery() {
  DHCPMessage *dhcp = kmalloc(sizeof(DHCPMessage));
  dhcp->op = 1;
  dhcp->htype = 1;
  dhcp->hw_addr_len = 6;
  dhcp->hops = 0;
  u32 txid = rand() + rand() + rand();
  dhcp->transaction_id = txid;
  dhcp->secs = 0;
  dhcp->flags = 0;
  dhcp->client_ip = 0;
  dhcp->your_ip = 0;
  dhcp->server_ip = 0;
  dhcp->gateway_ip = 0;
  memcopy(rtl8139_device.mac_addr, (byte *)&dhcp->client_mac, 6);
  memset(dhcp->zeroes, 0, sizeof(dhcp->zeroes));
  dhcp->magic_cookie = switch_endian32(0x63825363);
  dhcp->option1.data = 1;
  dhcp->option1.type = 53;
  dhcp->option1.len = 1;
  dhcp->end_option = 0xff;
  return dhcp;
}

void send_dchp_discovery() {
  byte src_ip[] = {0, 0, 0, 0};
  byte dest_ip[] = {255, 255, 255, 255};
  u32 tot_size = sizeof(IPv4Header) + sizeof(UDP) + sizeof(DHCPMessage);

  DHCPMessage *dchp_message = create_dchp_discovery();
  IPv4Header *ipv4 = create_ipv4_header(src_ip, dest_ip, tot_size);
  UDP *udp = create_udp_packet(68, 67, sizeof(DHCPMessage) + 8, 0);

  byte eth_dst[] = {0xff, 0xff, 0xff, 0xff, 0xff, 0xff};
  byte *data = kmalloc(tot_size);
  memcopy((byte *)ipv4, data, sizeof(IPv4Header));
  memcopy((byte *)udp, data + sizeof(IPv4Header), sizeof(UDP));
  memcopy((byte *)dchp_message, data + sizeof(IPv4Header) + sizeof(UDP),
          sizeof(DHCPMessage));
  send_ethernet_packet(eth_dst, ETH_IPV4, data, tot_size);
  kfree(data);
  kfree(dchp_message);
  kfree(udp);
  kfree(ipv4);
}