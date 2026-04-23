/***************************************************************************************
 * Copyright (c) 2026 Beijing Institute of Open Source Chip (BOSC)
 * Copyright (c) 2026 Institute of Computing Technology, Chinese Academy of
 *Sciences
 *
 * NEMU is licensed under Mulan PSL v2.
 * You can use this software according to the terms and conditions of the Mulan
 *PSL v2. You may obtain a copy of Mulan PSL v2 at:
 *          http://license.coscl.org.cn/MulanPSL2
 *
 * THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY
 *KIND, EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO
 *NON-INFRINGEMENT, MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE.
 *
 * See the Mulan PSL v2 for more details.
 ***************************************************************************************/

#include <utils.h>
#include <device/map.h>
#include <memory/host.h>

#define UART_RBR 0x0
#define UART_THR 0x0
#define UART_DLL 0x0
#define UART_IER 0x1
#define UART_DLH 0x1
#define UART_IIR 0x2
#define UART_FCR 0x2
#define UART_LCR 0x3
#define UART_MCR 0x4
#define UART_LSR 0x5
#define UART_MSR 0x6
#define UART_SCR 0x7

#define UART_STANDARD_REG_NUM 8
#if defined(CONFIG_UART16550_REG_SHIFT_0)
#define UART_REG_SHIFT 0
#elif defined(CONFIG_UART16550_REG_SHIFT_2)
#define UART_REG_SHIFT 2
#else
#error "uart16550 regshift choice is not configured"
#endif
#define UART_REG_STRIDE (1u << UART_REG_SHIFT)
#define UART_REG_OFFSET(reg) ((reg) << UART_REG_SHIFT)

#if UART_REG_SHIFT != 0 && UART_REG_SHIFT != 2
#error "uart16550 only supports regshift 0 or 2"
#endif

#define UART_DW_USR     0x7c
#define UART_DW_DMASA   0xa8
#define UART_DW_TCR     0xac
#define UART_DW_DE_EN   0xb0
#define UART_DW_RE_EN   0xb4
#define UART_DW_DLF     0xc0
#define UART_DW_RAR     0xc4
#define UART_DW_TAR     0xc8
#define UART_DW_LCR_EXT 0xcc
#define UART_DW_CPR     0xf4
#define UART_DW_UCV     0xf8

#define UART16550_PIO_SIZE  0x20
#define UART16550_MMIO_SIZE 0x1000

#define UART_LCR_DLAB 0x80

#define UART_IIR_NO_INT 0x01
#define UART_IIR_FE 0xC0

#define UART_FCR_FE  0x01
#define UART_FCR_RFR 0x02

#define UART_LSR_DR   0x01
#define UART_LSR_THRE 0x20
#define UART_LSR_TEMT 0x40

#define UART_MSR_DCD 0x80
#define UART_MSR_DSR 0x20
#define UART_MSR_CTS 0x10

#define UART_DW_USR_TFNF 0x02
#define UART_DW_USR_TFE  0x04
#define UART_DW_USR_RFNE 0x08

#define UART_DW_DLF_MASK      0x3f
#define UART_DW_CPR_FIFO_16   (1u << 16)
#define UART_DW_UCV_DEFAULT   0x3430312aU

static uint8_t *serial_base = NULL;
static uint16_t divisor = 0x000c; // default to 9600 baud for 115200 base
static uint8_t fcr = 0;

#ifdef CONFIG_UART16550_INPUT_FIFO
#include <errno.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#define QUEUE_SIZE 1024
static char queue[QUEUE_SIZE] = {};
static int f = 0, r = 0;
#define FIFO_PATH "/tmp/nemu-serial"
static int fifo_fd = -1;

static void serial_enqueue(char ch) {
  int next = (r + 1) % QUEUE_SIZE;
  if (next != f) {
    queue[r] = ch;
    r = next;
  }
}

static char serial_dequeue() {
  char ch = 0;
  if (f != r) {
    ch = queue[f];
    f = (f + 1) % QUEUE_SIZE;
  }
  return ch;
}

static void serial_poll_input() {
  if (fifo_fd < 0) {
    return;
  }
  if (f == r) {
    char input[256];
    int ret = read(fifo_fd, input, sizeof(input));
    if (ret > 0) {
      for (int i = 0; i < ret; i++) {
        serial_enqueue(input[i]);
      }
    }
  }
}

static void serial_fifo_reset() {
  f = r = 0;
}

static void init_fifo() {
  int ret = mkfifo(FIFO_PATH, 0666);
  assert(ret == 0 || errno == EEXIST);
  fifo_fd = open(FIFO_PATH, O_RDONLY | O_NONBLOCK);
  assert(fifo_fd != -1);
}

#endif

static inline uint8_t serial_read8(uint32_t offset) {
  return serial_base[offset];
}

static inline void serial_write8(uint32_t offset, uint8_t value) {
  serial_base[offset] = value;
}

static inline uint32_t serial_read32(uint32_t offset) {
  return host_read(serial_base + offset, 4);
}

static inline void serial_write32(uint32_t offset, uint32_t value) {
  host_write(serial_base + offset, 4, value);
}

static inline bool serial_decode_standard_reg(uint32_t offset, uint32_t *reg) {
  if ((offset & (UART_REG_STRIDE - 1)) != 0) {
    return false;
  }

  uint32_t shifted_reg = offset >> UART_REG_SHIFT;
  if (shifted_reg < UART_STANDARD_REG_NUM) {
    *reg = shifted_reg;
    return true;
  }

  return false;
}

static inline void serial_update_status() {
  uint8_t lsr = UART_LSR_THRE | UART_LSR_TEMT;
  uint32_t usr = UART_DW_USR_TFNF | UART_DW_USR_TFE;

#ifdef CONFIG_UART16550_INPUT_FIFO
  serial_poll_input();
  if (f != r) {
    lsr |= UART_LSR_DR;
    usr |= UART_DW_USR_RFNE;
  }
#endif

  serial_write8(UART_REG_OFFSET(UART_LSR), lsr);
  serial_write32(UART_DW_USR, usr);
}

static void serial_handle_standard_access(uint32_t reg, bool is_write) {
  switch (reg) {
    case UART_THR:
      if (serial_read8(UART_REG_OFFSET(UART_LCR)) & UART_LCR_DLAB) {
        if (is_write) {
          divisor = (divisor & 0xff00) | serial_read8(UART_REG_OFFSET(UART_DLL));
        } else {
          serial_write8(UART_REG_OFFSET(UART_DLL), (uint8_t)(divisor & 0xff));
        }
      } else if (is_write) {
        putc(serial_read8(UART_REG_OFFSET(UART_THR)), stderr);
      } else {
#ifdef CONFIG_UART16550_INPUT_FIFO
        serial_poll_input();
        if (f != r) {
          serial_write8(UART_REG_OFFSET(UART_RBR), (uint8_t)serial_dequeue());
        } else {
          serial_write8(UART_REG_OFFSET(UART_RBR), 0);
        }
#else
        serial_write8(UART_REG_OFFSET(UART_RBR), 0);
#endif
      }
      serial_update_status();
      break;

    case UART_IER:
      if (serial_read8(UART_REG_OFFSET(UART_LCR)) & UART_LCR_DLAB) {
        if (is_write) {
          divisor = (divisor & 0x00ff) |
                    ((uint16_t)serial_read8(UART_REG_OFFSET(UART_DLH)) << 8);
        } else {
          serial_write8(UART_REG_OFFSET(UART_DLH), (uint8_t)(divisor >> 8));
        }
      } else if (is_write) {
        serial_write8(UART_REG_OFFSET(UART_IER),
                      serial_read8(UART_REG_OFFSET(UART_IER)) & 0x0f);
      }
      break;

    case UART_IIR:
      if (is_write) {
        uint8_t val = serial_read8(UART_REG_OFFSET(UART_FCR));
        fcr = val;
#ifdef CONFIG_UART16550_INPUT_FIFO
        if (val & UART_FCR_RFR) {
          serial_fifo_reset();
        }
#endif
        serial_update_status();
      } else {
        uint8_t iir = UART_IIR_NO_INT;
        if (fcr & UART_FCR_FE) {
          iir |= UART_IIR_FE;
        }
        serial_write8(UART_REG_OFFSET(UART_IIR), iir);
      }
      break;

    case UART_LCR:
    case UART_MCR:
      break;

    case UART_LSR:
      if (!is_write) {
        serial_update_status();
      }
      break;

    case UART_MSR:
      if (!is_write) {
        serial_write8(UART_REG_OFFSET(UART_MSR), UART_MSR_DCD | UART_MSR_DSR | UART_MSR_CTS);
      }
      break;

    case UART_SCR:
      break;

    default:
      break;
  }
}

static void serial_handle_dw_access(uint32_t offset, bool is_write) {
  switch (offset) {
    case UART_DW_USR:
      if (!is_write) {
        serial_update_status();
      }
      break;

    case UART_DW_DLF:
      if (is_write) {
        serial_write32(UART_DW_DLF, serial_read32(UART_DW_DLF) & UART_DW_DLF_MASK);
      }
      break;

    case UART_DW_CPR:
      if (!is_write) {
        serial_write32(UART_DW_CPR, UART_DW_CPR_FIFO_16);
      }
      break;

    case UART_DW_UCV:
      if (!is_write) {
        serial_write32(UART_DW_UCV, UART_DW_UCV_DEFAULT);
      }
      break;

    case UART_DW_DMASA:
    case UART_DW_TCR:
    case UART_DW_DE_EN:
    case UART_DW_RE_EN:
    case UART_DW_RAR:
    case UART_DW_TAR:
    case UART_DW_LCR_EXT:
      break;

    default:
      break;
  }
}

static void serial_io_handler(uint32_t offset, int len, bool is_write) {
  uint32_t reg = 0;

  assert(len >= 1 && len <= 8);
  if (serial_decode_standard_reg(offset, &reg)) {
    serial_handle_standard_access(reg, is_write);
    return;
  }
  serial_handle_dw_access(offset, is_write);
}

void init_uart16550() {
  serial_base = new_space(UART16550_MMIO_SIZE);
  fcr = 0;

  serial_write8(UART_REG_OFFSET(UART_IIR), UART_IIR_NO_INT);
  serial_write8(UART_REG_OFFSET(UART_MSR), UART_MSR_DCD | UART_MSR_DSR | UART_MSR_CTS);
  serial_write32(UART_DW_DLF, 0);
  serial_write32(UART_DW_DMASA, 0);
  serial_write32(UART_DW_TCR, 0);
  serial_write32(UART_DW_DE_EN, 0);
  serial_write32(UART_DW_RE_EN, 0);
  serial_write32(UART_DW_RAR, 0);
  serial_write32(UART_DW_TAR, 0);
  serial_write32(UART_DW_LCR_EXT, 0);
  serial_update_status();

#ifdef CONFIG_UART16550_PORT
  add_pio_map("uart16550", CONFIG_UART16550_PORT, serial_base, UART16550_PIO_SIZE, serial_io_handler);
#endif
  add_mmio_map("uart16550", CONFIG_UART16550_MMIO, serial_base, UART16550_MMIO_SIZE, serial_io_handler);

#ifdef CONFIG_UART16550_INPUT_FIFO
  init_fifo();
#endif
}
