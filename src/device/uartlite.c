#include <device/map.h>

#define CH_OFFSET 0
#define UARTLITE_RX_FIFO  0x0
#define UARTLITE_TX_FIFO  0x4
#define UARTLITE_STAT_REG 0x8
#define UARTLITE_CTRL_REG 0xc

#define UARTLITE_RST_FIFO 0x03
#define UARTLITE_TX_FULL  0x08
#define UARTLITE_RX_VALID 0x01

// #define QUEUE_SIZE 1024
// static char queue[QUEUE_SIZE] = {};
// static int f = 0, r = 0;

static uint8_t *serial_base = NULL;

// static void serial_enqueue(char ch) {
//   int next = (r + 1) % QUEUE_SIZE;
//   if (next != f) {
//     // not full
//     queue[r] = ch;
//     r = next;
//   }
// }

// static char serial_dequeue() {
//   char ch = 0xff;

//   extern uint32_t uptime();
//   static uint32_t last = 0;
//   uint32_t now = uptime();
//   if (now > last) {
//     Log("now = %d", now);
//     last = now;
//   }
//   // 90s after starting
//   if (now > 90) {
//     if (f != r) {
//       ch = queue[f];
//       f = (f + 1) % QUEUE_SIZE;
//     }
//   }
//   return ch;
// }

static void serial_io_handler(uint32_t offset, int len, bool is_write) {
  assert(len == 1);
  switch (offset) {
    /* We bind the serial port with the host stdout in NEMU. */
    case UARTLITE_TX_FIFO:
      if (is_write) putc(serial_base[UARTLITE_TX_FIFO], stderr);
      else panic("Cannot read UARTLITE_TX_FIFO");
      break;
    case UARTLITE_STAT_REG:
      if (!is_write) serial_base[UARTLITE_STAT_REG] = 0x0;
      break;
  }
}

// static void preset_input() {
//   char buf[] = debian_cmd;
//   int i;
//   for (i = 0; i < strlen(buf); i ++) {
//     serial_enqueue(buf[i]);
//   }
// }

void init_uartlite() {
  serial_base = new_space(0xd);
  add_pio_map("uartlite", SERIAL_PORT, serial_base, 0xd, serial_io_handler);
  add_mmio_map("uartlite", SERIAL_MMIO, serial_base, 0xd, serial_io_handler);

  // preset_input();
}
