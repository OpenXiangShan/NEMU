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

#define UART_REG_OFFSET(reg) ((reg) << 2)

#define UART_LCR_DLAB 0x80

#define UART_IIR_NO_INT 0x01
#define UART_IIR_FE 0xC0

#define UART_FCR_FE 0x01
#define UART_FCR_RFR 0x02
#define UART_FCR_XFR 0x04

#define UART_LSR_DR   0x01
#define UART_LSR_THRE 0x20
#define UART_LSR_TEMT 0x40

#define UART_MSR_DCD 0x80
#define UART_MSR_DSR 0x20
#define UART_MSR_CTS 0x10

static uint8_t *serial_base = NULL;
static uint16_t divisor = 0x000c; // default to 9600 baud for 115200 base

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

static inline void serial_update_lsr() {
	serial_base[UART_REG_OFFSET(UART_LSR)] |= UART_LSR_THRE | UART_LSR_TEMT;
#ifdef CONFIG_SERIAL_INPUT_FIFO
	serial_poll_input();
	if (f != r) {
		serial_base[UART_REG_OFFSET(UART_LSR)] |= UART_LSR_DR;
	} else {
		serial_base[UART_REG_OFFSET(UART_LSR)] &= ~UART_LSR_DR;
	}
#else
	serial_base[UART_REG_OFFSET(UART_LSR)] &= ~UART_LSR_DR;
#endif
}

static void serial_handle_access(uint32_t offset, int len, bool is_write) {
	uint32_t reg = (offset >> 2) & 0x7; // reg-shift = 2
	switch (reg) {
		case UART_THR:
			if (serial_base[UART_REG_OFFSET(UART_LCR)] & UART_LCR_DLAB) {
				if (is_write) {
					divisor = (divisor & 0xff00) | serial_base[UART_REG_OFFSET(UART_DLL)];
				} else {
					serial_base[UART_REG_OFFSET(UART_DLL)] = (uint8_t)(divisor & 0xff);
				}
			} else {
				if (is_write) {
					#ifndef CONFIG_SHARE
					putc(serial_base[UART_REG_OFFSET(UART_THR)], stderr);
					#endif // CONFIG_SHARE
					serial_update_lsr();
				} else {
#ifdef CONFIG_SERIAL_INPUT_FIFO
					serial_poll_input();
					if (f != r) {
						serial_base[UART_REG_OFFSET(UART_RBR)] = (uint8_t)serial_dequeue();
					} else {
						serial_base[UART_REG_OFFSET(UART_RBR)] = 0;
					}
#else
					serial_base[UART_REG_OFFSET(UART_RBR)] = 0;
#endif
					serial_update_lsr();
				}
			}
			break;

		case UART_IER:
			if (serial_base[UART_REG_OFFSET(UART_LCR)] & UART_LCR_DLAB) {
				if (is_write) {
					divisor = (divisor & 0x00ff) | ((uint16_t)serial_base[UART_REG_OFFSET(UART_DLH)] << 8);
				} else {
					serial_base[UART_REG_OFFSET(UART_DLH)] = (uint8_t)(divisor >> 8);
				}
			} else {
				if (is_write) {
					serial_base[UART_REG_OFFSET(UART_IER)] &= 0x0f;
				}
			}
			break;

		case UART_IIR:
			if (is_write) {
				uint8_t val = serial_base[UART_REG_OFFSET(UART_FCR)];
				serial_base[UART_REG_OFFSET(UART_FCR)] = val;
#ifdef CONFIG_SERIAL_INPUT_FIFO
				if (val & UART_FCR_RFR) {
					serial_fifo_reset();
				}
#endif
				serial_update_lsr();
			} else {
				uint8_t iir = UART_IIR_NO_INT;
				if (serial_base[UART_REG_OFFSET(UART_FCR)] & UART_FCR_FE) {
					iir |= UART_IIR_FE;
				}
				serial_base[UART_REG_OFFSET(UART_IIR)] = iir;
			}
			break;

		case UART_LCR:
			if (!is_write) {
				// no side effects
			}
			break;

		case UART_MCR:
			if (!is_write) {
				// no side effects
			}
			break;

		case UART_LSR:
			if (!is_write) {
				serial_update_lsr();
			}
			break;

		case UART_MSR:
			if (!is_write) {
				// fixed modem status
				serial_base[UART_REG_OFFSET(UART_MSR)] = UART_MSR_DCD | UART_MSR_DSR | UART_MSR_CTS;
			}
			break;

		case UART_SCR:
			// read/write scratch
			break;

		default:
			break;
	}
}

static void serial_io_handler(uint32_t offset, int len, bool is_write) {
	assert(len >= 1 && len <= 8);
	serial_handle_access(offset, len, is_write);
}

void init_serial() {
	serial_base = new_space(0x20);
	serial_base[UART_REG_OFFSET(UART_LSR)] = UART_LSR_THRE | UART_LSR_TEMT;
	serial_base[UART_REG_OFFSET(UART_IIR)] = UART_IIR_NO_INT;
	serial_base[UART_REG_OFFSET(UART_MSR)] = UART_MSR_DCD | UART_MSR_DSR | UART_MSR_CTS;

#ifdef CONFIG_SERIAL_PORT
	add_pio_map("serial", CONFIG_SERIAL_PORT, serial_base, 0x20, serial_io_handler);
#endif
	add_mmio_map("serial", CONFIG_SERIAL_MMIO, serial_base, 0x20, serial_io_handler);

#ifdef CONFIG_SERIAL_INPUT_FIFO
	init_fifo();
#endif
}
