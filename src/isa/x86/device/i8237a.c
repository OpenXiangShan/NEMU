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

#define I8237A_PAGE_0_PORT 0x87

static uint8_t *i8237a_base = NULL;

static void i8237a_io_handler(uint32_t offset, int len, bool is_write) {
}

void init_i8237a() {
  i8237a_base = (void *)new_space(1);
  add_pio_map("i8237a-page0", I8237A_PAGE_0_PORT, i8237a_base, 1, i8237a_io_handler);
  i8237a_base[0] = 0xff;
}
