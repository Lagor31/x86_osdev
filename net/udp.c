#include "udp.h"
#include "../mem/mem.h"
#include "../net/ethernet.h"
#include "../net/net.h"

UDP* create_udp_packet(u16 src_port, u16 dest_port, u16 len, u16 checksum) {
  UDP* udp = kmalloc(sizeof(UDP));
  udp->src_port = switch_endian16(src_port);
  udp->dst_port = switch_endian16(dest_port);
  udp->len = switch_endian16(len);
  udp->checksum = switch_endian16(checksum);
  return udp;
}
