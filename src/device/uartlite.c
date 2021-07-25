#include <utils.h>
#include <device/map.h>

#define CH_OFFSET 0
#define UARTLITE_RX_FIFO  0x0
#define UARTLITE_TX_FIFO  0x4
#define UARTLITE_STAT_REG 0x8
#define UARTLITE_CTRL_REG 0xc

#define UARTLITE_RST_FIFO 0x03
#define UARTLITE_TX_FULL  0x08
#define UARTLITE_RX_VALID 0x01

static uint8_t *serial_base = NULL;

#ifdef CONFIG_UARTLITE_INPUT_FIFO
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <errno.h>

#define QUEUE_SIZE 1024
static char queue[QUEUE_SIZE] = {};
static int f = 0, r = 0;
#define FIFO_PATH "/tmp/nemu-serial"
static int fifo_fd = 0;

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
  if (f != r) {
    ch = queue[f];
    f = (f + 1) % QUEUE_SIZE;
  }
  return ch;
}

static inline uint8_t serial_rx_ready_flag() {
  static uint32_t last = 0; // unit: s
  uint32_t now = get_time() / 1000000;
  if (now > last) {
    Log("now = %d", now);
    last = now;
  }

  if (f == r) {
    char input[256];
    // First open in read only and read
    int ret = read(fifo_fd, input, 256);
    assert(ret < 256);

    if (ret > 0) {
      int i;
      for (i = 0; i < ret; i ++) {
        serial_enqueue(input[i]);
      }
    }
  }
  return (f == r ? 0 : LSR_RX_READY);
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

static void init_fifo() {
  int ret = mkfifo(FIFO_PATH, 0666);
  assert(ret == 0 || errno == EEXIST);
  fifo_fd = open(FIFO_PATH, O_RDONLY | O_NONBLOCK);
  assert(fifo_fd != -1);
}

#endif

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

void init_uartlite() {
  serial_base = new_space(0xd);
  add_pio_map("uartlite", CONFIG_UARTLITE_PORT, serial_base, 0xd, serial_io_handler);
  add_mmio_map("uartlite", CONFIG_UARTLITE_MMIO, serial_base, 0xd, serial_io_handler);

#ifdef CONFIG_UARTLITE_INPUT_FIFO
  init_fifo();
  preset_input();
#endif
}
