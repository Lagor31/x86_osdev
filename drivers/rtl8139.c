#include "pci.h"
#include "rtl8139.h"
#include "screen.h"
#include "../mem/mem.h"
#include "../kernel/kernel.h"
#include "../lib/utils.h"
#include "../proc/thread.h"
#include "../mem/buddy.h"

pci_dev_t pci_rtl8139_device;
rtl8139_dev_t rtl8139_device;
u8 TSAD_array[4] = {0x20, 0x24, 0x28, 0x2C};
u8 TSD_array[4] = {0x10, 0x14, 0x18, 0x1C};
u32 current_packet_ptr;
u32 packets = 0;
u32 rtl8139_init() {
  pci_rtl8139_device = pci_get_device(RTL8139_VENDOR_ID, RTL8139_DEVICE_ID, -1);
  uint32_t ret = pci_read(pci_rtl8139_device, PCI_BAR0);
  rtl8139_device.bar_type = ret & 0x1;
  // Get io base or mem base by extracting the high 28/30 bits
  rtl8139_device.io_base = ret & (~0x3);
  rtl8139_device.mem_base = ret & (~0xf);
  kprintf("rtl8139 use %s access (base: %x)\n",
          (rtl8139_device.bar_type == 0) ? "mem based" : "port based",
          (rtl8139_device.bar_type != 0) ? rtl8139_device.io_base
                                         : rtl8139_device.mem_base);
  // Set current TSAD
  rtl8139_device.tx_cur = 0;

  // Enable PCI Bus Mastering
  uint32_t pci_command_reg = pci_read(pci_rtl8139_device, PCI_COMMAND);
  if (!(pci_command_reg & (1 << 2))) {
    pci_command_reg |= (1 << 2);
    pci_write(pci_rtl8139_device, PCI_COMMAND, pci_command_reg);
  }

  // Send 0x00 to the CONFIG_1 register (0x52) to set the LWAKE + LWPTN to
  // active high. this should essentially *power on* the device.
  outb(rtl8139_device.io_base + 0x52, 0x0);

  // Soft reset
  outb(rtl8139_device.io_base + 0x37, 0x10);
  while ((inb(rtl8139_device.io_base + 0x37) & 0x10) != 0) {
    // Do nothibg here...
  }

  // Allocate receive buffer
  rtl8139_device.rx_buffer = kmalloc(8192 + 16 + 1500);
  memset((byte *)rtl8139_device.rx_buffer, 0x0, 8192 + 16 + 1500);
  outdw(rtl8139_device.io_base + 0x30, (uint32_t)PA(rtl8139_device.rx_buffer));

  // Sets the TOK and ROK bits high
  outw(rtl8139_device.io_base + 0x3C, 0x0005);

  // (1 << 7) is the WRAP bit, 0xf is AB+AM+APM+AAP, 0xe = Not promiscuous

  outdw(rtl8139_device.io_base + 0x44, RTL_APM | RTL_AB | RTL_AM | (1 << 7));

  // Sets the RE and TE bits high
  outb(rtl8139_device.io_base + 0x37, 0x0C);

  uint32_t mac_part1 = indw(rtl8139_device.io_base + 0x00);
  uint16_t mac_part2 = inw(rtl8139_device.io_base + 0x04);
  rtl8139_device.mac_addr[0] = mac_part1 >> 0;
  rtl8139_device.mac_addr[1] = mac_part1 >> 8;
  rtl8139_device.mac_addr[2] = mac_part1 >> 16;
  rtl8139_device.mac_addr[3] = mac_part1 >> 24;

  rtl8139_device.mac_addr[4] = mac_part2 >> 0;
  rtl8139_device.mac_addr[5] = mac_part2 >> 8;
  kprintf("MAC Address: %01x:%01x:%01x:%01x:%01x:%01x\n",
          rtl8139_device.mac_addr[0], rtl8139_device.mac_addr[1],
          rtl8139_device.mac_addr[2], rtl8139_device.mac_addr[3],
          rtl8139_device.mac_addr[4], rtl8139_device.mac_addr[5]);

  uint32_t irq_num = pci_read(pci_rtl8139_device, PCI_INTERRUPT_LINE);
  kprintf("IRQ Number: %d\n", irq_num);
  return irq_num;
}

void rtl8139_packet_handler(registers_t *regs) {
  // kprintf("RTL8139 interrupt was fired !!!! \n");
  uint16_t status = inw(rtl8139_device.io_base + 0x3e);

  if (status & TOK) {
    kprintf("Packet sent\n");
    // if (!(status & ROK)) outw(rtl8139_device.io_base + 0x3E, 0x5);
  }
  if (status & ROK) {
    // kprintf("Received packet\n");
    receive_packet();
    // outw(rtl8139_device.io_base + 0x3E, 0x5);
  }
  outw(rtl8139_device.io_base + 0x3E, 0x5);

  UNUSED(regs);
}

void rtl8139_send_packet(void *data, uint32_t len) {
  // First, copy the data to a physically contiguous chunk of memory
  void *transfer_data = kmalloc(len);
  void *phys_addr = PA(transfer_data);
  memcopy(data, transfer_data, len);

  // Second, fill in physical address of data, and length
  outdw(rtl8139_device.io_base + TSAD_array[rtl8139_device.tx_cur],
        (uint32_t)phys_addr);
  outdw(rtl8139_device.io_base + TSD_array[rtl8139_device.tx_cur++], len);
  if (rtl8139_device.tx_cur > 3) rtl8139_device.tx_cur = 0;
}

void receive_packet() {
  /* u32 packets = 0;
  u32 total_size = 0; */

  //if (!(inb(rtl8139_device.io_base + 0x37) & RTL_BUFE)) return;

  do {
    uint16_t *t = (uint16_t *)(rtl8139_device.rx_buffer + current_packet_ptr);
    // Skip packet header, get packet length
    uint16_t packet_length = *(t + 1);

    // Skip, packet header and packet length, now t points to the packet data
    t = t + 2;
    // qemu_printf("Printing packet at addr 0x%x\n", (uint32_t)t);
    // xxd(t, packet_length);

    // Now, ethernet layer starts to handle the packet(be sure to make a copy of
    // the packet, insteading of using the buffer) and probabbly this should be
    // done in a separate thread...

    /* for (u32 o = 0; o <= MAX_ORDER; ++o) {
      print_buddy_status(o, &fast_buddy_new);
    }
    kprintf("\n"); */
    void *packet = fmalloc(packet_length);
    // total_size += packet_length;

    memcopy((byte *)t, packet, packet_length);
    Work *net_work = fmalloc(sizeof(Work));
    net_work->data = packet;
    net_work->type = 0;
    net_work->t = NULL;
    net_work->size = packet_length;
    LIST_INIT(&net_work->work_queue);
    list_add_tail(&kwork_queue, &net_work->work_queue);
    /* ffree(packet);
    ffree(net_work); */

    ++packets;
    //print_ethernet_packet(packet);

    /* for (u32 i = 0; i < ALLOC_NUM; ++i) {
      u32 r = rand() % (MAX_ORDER - 8);
      // kprintf("%d) Size: %d -> ", i, r);
      get_buddy_new(r, &fast_buddy_new);
    } */
    if (packets > 100) {
      kprintfColor(LIGHTGREEN, "%d packets in while loop!\n", packets);
      print_ethernet_packet(packet);
    }
    current_packet_ptr =
        (current_packet_ptr + packet_length + 4 + 3) & RX_READ_POINTER_MASK;

    if (current_packet_ptr > RX_BUF_SIZE) current_packet_ptr -= RX_BUF_SIZE;

    outw(rtl8139_device.io_base + CAPR, current_packet_ptr - 0x10);

  } while ((inb(rtl8139_device.io_base + 0x37) & RTL_BUFE) == 0);

  /* if (packets >= 3 || total_size >= 200)
    kprintf("Packets: %d Size: %d\n", packets, total_size);
 */
  packets = 0;
  wake_up_thread(kwork_thread);
}

void print_mac_address() {
  kprintf("MAC Address: %01x:%01x:%01x:%01x:%01x:%01x\n",
          rtl8139_device.mac_addr[0], rtl8139_device.mac_addr[1],
          rtl8139_device.mac_addr[2], rtl8139_device.mac_addr[3],
          rtl8139_device.mac_addr[4], rtl8139_device.mac_addr[5]);
}