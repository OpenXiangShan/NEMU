/***************************************************************************************
* Copyright (c) 2014-2021 Zihao Yu, Nanjing University
* Copyright (c) 2020-2022 Institute of Computing Technology, Chinese Academy of Sciences
*
* NEMU is licensed under Mulan PSL v2.
* You can use this software according to the terms and conditions of the Mulan PSL v2.
* You may obtain a copy of Mulan PSL v2 at:
*          http://license.coscl.org.cn/MulanPSL2
*
* THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND,
* EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT,
* MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE.
*
* See the Mulan PSL v2 for more details.
***************************************************************************************/

#include <utils.h>
#include <device/map.h>

/* http://en.wikibooks.org/wiki/Serial_Programming/8250_UART_Programming */
// NOTE: this is compatible to 16550

#define CH_OFFSET 0
#define LSR_OFFSET 5
#define LSR_TX_READY 0x20
#define LSR_FIFO_EMPTY 0x40
#define LSR_RX_READY 0x01

static uint8_t *serial_base = NULL;

#include <stdio.h>

#ifdef CONFIG_SERIAL_INPUT_FIFO
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
    /* We bind the serial port with the host stderr in NEMU. */
    case CH_OFFSET:
      if (is_write) putc(serial_base[0], stderr);
      else serial_base[0] = MUXDEF(CONFIG_SERIAL_INPUT_FIFO, serial_dequeue(), 0xff);
      break;
    case LSR_OFFSET:
      if (!is_write)
        serial_base[5] = LSR_TX_READY | LSR_FIFO_EMPTY | MUXDEF(CONFIG_SERIAL_INPUT_FIFO, serial_rx_ready_flag(), 0);
      break;
  }
}

void init_serial() {
#ifdef CONFIG_SERIAL_UARTLITE
  void init_uartlite();
  init_uartlite();
  void init_uart_snps();
  init_uart_snps();
  // to avoid unused-function warning
  (void)serial_io_handler;
#else
  printf("init_serial\n");
  serial_base = new_space(8);
  add_pio_map ("serial", CONFIG_SERIAL_PORT, serial_base, 8, serial_io_handler);
  add_mmio_map("serial", CONFIG_SERIAL_MMIO, serial_base, 8, serial_io_handler);

#ifdef CONFIG_SERIAL_INPUT_FIFO
  init_fifo();
  preset_input();
#endif
#endif // CONFIG_SERIAL_UARTLITE
}
