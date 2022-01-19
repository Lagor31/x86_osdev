#include "ipv4.h"
#include "../mem/mem.h"
#include "net.h"
#include "ethernet.h"

static unsigned short compute_checksum(unsigned short *addr,
                                       unsigned int count) {
  register unsigned long sum = 0;
  while (count > 1) {
    sum += *addr++;
    count -= 2;
  }
  // if any bytes left, pad the bytes and add
  if (count > 0) {
    sum += ((*addr) & switch_endian16(0xFF00));
  }
  // Fold sum to 16 bits: add carrier to result
  while (sum >> 16) {
    sum = (sum & 0xffff) + (sum >> 16);
  }
  // one's complement
  sum = ~sum;
  return ((unsigned short)sum);
}

IPv4Header *create_ipv4_header(byte src_ip[], byte dest_ip[], u32 len) {
  IPv4Header *ip = kmalloc(sizeof(IPv4Header));
  ip->version = 4;
  ip->ihl = 5;

  ip->dscp = 0;
  ip->ecn = 0;
  ip->total_length = switch_endian16(len);
  ip->identification = 0;

  ip->flags = 0;
  ip->fragment_offset = 0;
  ip->header_checksum = 0;
  ip->ttl = 10;
  ip->protocol = 0x11;  // UDP
  ip->source[0] = src_ip[0];
  ip->source[1] = src_ip[1];
  ip->source[2] = src_ip[2];
  ip->source[3] = src_ip[3];
  ip->dest[0] = dest_ip[0];
  ip->dest[1] = dest_ip[1];
  ip->dest[2] = dest_ip[2];
  ip->dest[3] = dest_ip[3];

  // ip->header_checksum = (ipv4_checksum(ip, 4 * ip->ihl));
  ip->header_checksum = compute_checksum((u16 *)ip, 4 * ip->ihl);
  return ip;
}

int ipv4_checksum(u16 *addr, int len) {
  register int nleft = len;
  register u16 *w = addr;
  register int sum = 0;
  u16 answer = 0;

  /*
   * Our algorithm is simple, using a 32 bit accumulator (sum), we add
   * sequential 16 bit words to it, and at the end, fold back all the
   * carry bits from the top 16 bits into the lower 16 bits.
   */
  while (nleft > 1) {
    sum += *w++;
    nleft -= 2;
  }

  /* mop up an odd byte, if necessary */
  if (nleft == 1) {
    *(u8 *)(&answer) = *(u8 *)w;
    sum += answer;
  }

  /* add back carry outs from top 16 bits to low 16 bits */
  sum = (sum >> 16) + (sum & 0xffff); /* add hi 16 to low 16 */
  sum += (sum >> 16);                 /* add carry */
  answer = ~sum;                      /* truncate to 16 bits */
  return (answer);
}

void send_ip_packet(IPv4Header *ip) {
  byte brd[] = {0xff, 0xff, 0xff, 0xff, 0xff, 0xff};
  send_ethernet_packet(brd, ETH_IPV4, ip, sizeof(IPv4Header));
}