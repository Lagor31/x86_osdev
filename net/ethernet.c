#include "ethernet.h"
#include "../drivers/rtl8139.h"
#include "../mem/mem.h"
#include "../net/net.h"

void send_ethernet_packet(byte dst[], u16 eth_type, void *data_in,
                          u32 data_len) {
  u32 eth_frame_size = 6 + 6 + 2 + data_len;
  if (eth_frame_size < 60) eth_frame_size = 60;
  void *payload = kmalloc(eth_frame_size);
  memset(payload, 0, eth_frame_size);
  memcopy(dst, (byte *)payload, 6);
  memcopy(rtl8139_device.mac_addr, (byte *)payload + 6, 6);

  u16 type = switch_endian16(eth_type);
  memcopy((byte *)&type, (byte *)payload + 12, 2);
  memcopy((byte *)data_in, payload + 14, data_len);
  rtl8139_send_packet(payload, eth_frame_size);
  kfree(payload);
}