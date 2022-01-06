#include "net.h"
#include "../drivers/rtl8139.h"
#include "../cpu/isr.h"
#include "../drivers/screen.h"

/*
 We only support RTL8139
*/
void init_networking() {
  u32 net_irq = rtl8139_init();
  register_interrupt_handler(net_irq + 32, rtl8139_packet_handler);
}

bool is_equal_macs(byte *mac1, byte *mac2) {
  for (u8 i = 0; i < 6; ++i)
    if (mac1[i] != mac2[i]) return FALSE;

  return TRUE;
}

void print_ethernet_packet(void *packet) {
  byte *p = (byte *)packet;
  if (!is_equal_macs(&p[0], rtl8139_device.mac_addr)) return;
  kprintf("Packet Address: 0x%x\n", packet);
  kprintf("MAC Address Src: %01x:%01x:%01x:%01x:%01x:%01x\n", p[6], p[7], p[8],
          p[9], p[10], p[11]);
  kprintf("MAC Address Dst: %01x:%01x:%01x:%01x:%01x:%01x\n", p[0], p[1], p[2],
          p[3], p[4], p[5]);
  kprintf("Type: %02x:%02x\n", p[12], p[13]);

  if (p[12] == 0x08 && p[13] == 0) {
    kprintf("IPv4 Packet arrived - ");
    kprintf("Type: %d\n", p[23]);
    kprintf("IP Src: %d.%d.%d.%d\n", p[26], p[27], p[28], p[29]);
    kprintf("IP Dst: %d.%d.%d.%d\n", p[30], p[31], p[32], p[33]);
  }
  kprintf("\n");
}