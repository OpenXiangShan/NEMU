#include <device/map.h>

/* http://en.wikibooks.org/wiki/Serial_Programming/8250_UART_Programming */
// NOTE: this is compatible to 16550

#define CH_OFFSET 0
#define LSR_OFFSET 5
#define LSR_TX_READY 0x20
#define LSR_RX_READY 0x01

#define QUEUE_SIZE 1024
static char queue[QUEUE_SIZE] = {};
static int f = 0, r = 0;

static uint8_t *serial_base = NULL;

static void serial_enqueue(char ch) {
  int next = (r + 1) % QUEUE_SIZE;
  if (next != f) {
    // not full
    queue[r] = ch;
    r = next;
  }
}

static char serial_dequeue() {
  char ch = 0xff;

  extern uint32_t uptime();
  static uint32_t last = 0;
  uint32_t now = uptime();
  if (now > last) {
    Log("now = %d", now);
    last = now;
  }
  // 90s after starting
  if (now > 90) {
    if (f != r) {
      ch = queue[f];
      f = (f + 1) % QUEUE_SIZE;
    }
  }
  return ch;
}

static inline uint8_t serial_rx_ready_flag(void) {
  return (f == r ? 0 : LSR_RX_READY);
}

static void serial_io_handler(uint32_t offset, int len, bool is_write) {
  assert(len == 1);
  switch (offset) {
    /* We bind the serial port with the host stdout in NEMU. */
    case CH_OFFSET:
      if (is_write) putc(serial_base[0], stderr);
      else serial_base[0] = serial_dequeue();
      break;
    case LSR_OFFSET:
      if (!is_write) serial_base[5] = LSR_TX_READY | serial_rx_ready_flag();
      break;
  }
}

#define rt_thread_cmd "memtrace\n"
#define busybox_cmd "ls\n" \
  "cd /root\n" \
  "echo hello2\n" \
  "cd /root/benchmark\n" \
  "./stream\n" \
  "echo hello3\n" \
  "cd /root/redis\n" \
  "ls\n" \
  "ifconfig -a\n" \
  "ls\n" \
  "./redis-server\n" \

#define debian_cmd "root\n" \

static void preset_input() {
  char buf[] = debian_cmd;
  int i;
  for (i = 0; i < strlen(buf); i ++) {
    serial_enqueue(buf[i]);
  }
}

void init_serial() {
  serial_base = new_space(8);
  add_pio_map("serial", SERIAL_PORT, serial_base, 8, serial_io_handler);
  add_mmio_map("serial", SERIAL_MMIO, serial_base, 8, serial_io_handler);

  preset_input();
}
