#include <utils.h>
#include <device/map.h>

// #define CH_OFFSET 0
// #define UARTLITE_RX_FIFO  0x0
// #define UARTLITE_TX_FIFO  0x4
// #define UARTLITE_STAT_REG 0x8
// #define UARTLITE_CTRL_REG 0xc

// #define UARTLITE_RST_FIFO 0x03
// #define UARTLITE_TX_FULL  0x08
// #define UARTLITE_RX_VALID 0x01

#ifndef _UART_REG_H_
#define _UART_REG_H_

// TODO: add addr to CONFIG
#define           UART0_BASE	  0x1f00050000	     //Size=  64K	 Max_offset=  0x00010000
#define           UART_BASE	  UART0_BASE          //Size=  64K	 Max_offset=  0x00010000
/*uart register definitions*/
#define SYNC_ADDR 0

#define RBR       0x00 // + 0x10000*UART_NUM + UART_BASE
#define THR       0x00 // + 0x10000*UART_NUM + UART_BASE
#define DLL       0x00 // + 0x10000*UART_NUM + UART_BASE
#define DLH       0x04 // + 0x10000*UART_NUM + UART_BASE
#define IER       0x04 // + 0x10000*UART_NUM + UART_BASE
#define IIR       0x08 // + 0x10000*UART_NUM + UART_BASE
#define FCR       0x08 // + 0x10000*UART_NUM + UART_BASE
#define LCR       0x0c // + 0x10000*UART_NUM + UART_BASE
#define MCR       0x10 // + 0x10000*UART_NUM + UART_BASE
#define LSR       0x14 // + 0x10000*UART_NUM + UART_BASE
#define MSR       0x18 // + 0x10000*UART_NUM + UART_BASE
#define SCR_UART  0x1c // + 0x10000*UART_NUM + UART_BASE
#define LPDLL     0x20 // + 0x10000*UART_NUM + UART_BASE
#define LPDLH     0x24 // + 0x10000*UART_NUM + UART_BASE
#define USR       0x7c // + 0x10000*UART_NUM + UART_BASE
#define HTX       0xa4 // + 0x10000*UART_NUM + UART_BASE
#define DMASA     0xa8 // + 0x10000*UART_NUM + UART_BASE
#define UCV       0xf8 // + 0x10000*UART_NUM + UART_BASE
#define CTR       0xfc // + 0x10000*UART_NUM + UART_BASE

#define RFE  0x80
#define TEMT 0x40
#define THRE 0x20
#define BI   0x10
#define FE   0X08
#define PE   0X04
#define OE   0X02
#define DR   0X01
#define DTR  0X01
#define RTS  0X02
#define AFCE 0X20
#define SIRE 0X40
#define CTS  0X10

#endif

#define UART_NUM 0

static uint8_t *serial_base = NULL;

#ifdef CONFIG_UART_SNPS_INPUT_FIFO
// Make uart input fifo, not used and tested for uart_snps
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <errno.h>

#include <stdio.h>

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
  // printf("len: %d\n", len);
  static int dll_config = 0;
  switch (offset) {
    /* We bind the serial port with the host stdout in NEMU. */
    case THR:
      if (is_write)
        if (dll_config == 0) dll_config = serial_base[DLL];
        else {
          // assert(len == 1);
          // assert((serial_base[THR] & 0xff) == 0);
          putc(serial_base[THR], stderr);
        }
      else panic("Cannot read UART_SNPS_TX_FIFO");
      break;
  }
}

void init_uart_snps() {
  serial_base = new_space(0x100);
  serial_base[LSR] = 0x60;
  serial_base[USR] = 0x0;
  add_mmio_map("uart_snps", UART0_BASE, serial_base, 0x100, serial_io_handler);

#ifdef CONFIG_UART_SNPS_INPUT_FIFO
  init_fifo();
  preset_input();
#endif
}
