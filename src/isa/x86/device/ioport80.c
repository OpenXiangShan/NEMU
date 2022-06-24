/***************************************************************************************
* Copyright (c) 2014-2021 Zihao Yu, Nanjing University
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

#include <device/map.h>

// this is an empty device in QEMU
#define IOPORT80_PORT 0x80

static uint8_t *ioport80_base = NULL;

static void ioport80_io_handler(uint32_t offset, int len, bool is_write) {
}

void init_ioport80() {
  ioport80_base = (void *)new_space(1);
  add_pio_map("ioport80", IOPORT80_PORT, ioport80_base, 1, ioport80_io_handler);
}
