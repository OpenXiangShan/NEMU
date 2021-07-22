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

static uint8_t *uartlite_base = NULL;

static void uartlite_io_handler(uint32_t offset, int len, bool is_write) {
  assert(len == 1);
  switch (offset) {
    /* We bind the serial port with the host stdout in NEMU. */
    case UARTLITE_TX_FIFO:
      if (is_write) putc(uartlite_base[UARTLITE_TX_FIFO], stderr);
      else panic("Cannot read UARTLITE_TX_FIFO");
      break;
    case UARTLITE_STAT_REG:
      if (!is_write) uartlite_base[UARTLITE_STAT_REG] = 0x0;
      break;
  }
}

void init_uartlite() {
  uartlite_base = new_space(0xd);
  add_pio_map("uartlite", CONFIG_SERIAL_PORT, uartlite_base, 0xd, uartlite_io_handler);
  add_mmio_map("uartlite", CONFIG_SERIAL_MMIO, uartlite_base, 0xd, uartlite_io_handler);
}

