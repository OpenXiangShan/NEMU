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

#include <utils.h>
#include <device/map.h>
#include "local-include/csr.h"

#define CLINT_MTIMECMP (0x4000 / sizeof(clint_base[0]))
#define CLINT_MTIME    (0xBFF8 / sizeof(clint_base[0]))

static uint64_t *clint_base = NULL;

void set_mtime(uint64_t new_value);
void update_riscv_timer();
void init_mtimer_regs(void * p_mtime, void * p_mtimecmp);

#ifdef CONFIG_LIGHTQS
uint64_t clint_snapshot, spec_clint_snapshot;

void clint_take_snapshot() {
  clint_snapshot = clint_base[CLINT_MTIME];
}

void clint_take_spec_snapshot() {
  spec_clint_snapshot = clint_base[CLINT_MTIME];
}

void clint_restore_snapshot(uint64_t restore_inst_cnt) {
  if (spec_clint_snapshot <= restore_inst_cnt) {
    clint_snapshot = spec_clint_snapshot;
  }
  clint_base[CLINT_MTIME] = clint_snapshot;
}
#endif // CONFIG_LIGHTQS

static void clint_io_handler(uint32_t offset, int len, bool is_write) {
#ifdef CONFIG_LIGHTQS_DEBUG
  printf("clint op write %d addr %x\n", is_write, offset);
#endif // CONFIG_LIGHTQS_DEBUG
  if (is_write && offset == CLINT_MTIME) {
    set_mtime(clint_base[CLINT_MTIME]);
  } else {
    update_riscv_timer();
  }
}

void init_clint() {
  clint_base = (uint64_t *)new_space(0x10000);
  init_mtimer_regs(&clint_base[CLINT_MTIME], &clint_base[CLINT_MTIMECMP]);
  add_mmio_map("clint", CONFIG_CLINT_MMIO, (uint8_t *)clint_base, 0x10000, clint_io_handler);
}
