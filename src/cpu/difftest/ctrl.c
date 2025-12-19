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

#include <isa.h>
#include <ext/amuctrl.h>
#include <ext/msync.h>
#include <memory/paddr.h>
#include <memory/host.h>
#include <memory/store_queue_wrapper.h>
#include <memory/sparseram.h>
#include <cpu/cpu.h>
#include <difftest.h>
#include <string.h>

extern void nemu_memcpy_helper(paddr_t nemu_addr, void *dut_buf, size_t n, bool direction, void* (*cpy_func)(void*, const void*, size_t));

// Callback function pointer for AmuCtrlIO events
void (*amu_ctrl_callback_)(amu_ctrl_event_t) = NULL;

void ctrl_init() {
  init_mem();
  init_isa();
}

// ctrl_memcpy_init
void ctrl_memcpy_init(paddr_t nemu_addr, void *dut_buf, size_t n, bool direction) {
#ifdef CONFIG_USE_SPARSEMM
  nemu_sparse_mem_copy(nemu_addr, dut_buf, n, direction);
#else
#ifdef CONFIG_LARGE_COPY
  nemu_memcpy_helper(nemu_addr, dut_buf, n, direction, nemu_large_memcpy);
#else
  nemu_memcpy_helper(nemu_addr, dut_buf, n, direction, memcpy);
#endif
#endif
}

// ctrl_exec
void ctrl_exec(uint64_t n) {
  cpu_exec(n);
}

int ctrl_status() {
  switch (nemu_state.state) {
    case NEMU_RUNNING: case NEMU_QUIT:
      return 0;
    case NEMU_END:
      return (nemu_state.halt_ret == 0) ? 6 : 1;
    default:
      return 1;
  }
}

// ctrl_info for debug
void ctrl_info(void *reg_buf) {
  if (reg_buf != NULL) {
    memcpy(reg_buf, &cpu, sizeof(CPU_state));
  }
}

// Register callback function for AmuCtrlIO events
void ctrl_register_amu_callback(void (*callback)(amu_ctrl_event_t)) {
  amu_ctrl_callback_ = callback;
}
