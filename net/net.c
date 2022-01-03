#include "net.h"

#include "../drivers/rtl8139.h"
#include "../cpu/isr.h"

/*
 We only support RTL8139
*/
void init_networking() {
  u32 net_irq = rtl8139_init();
  register_interrupt_handler(net_irq + 32, rtl8139_packet_handler);
}