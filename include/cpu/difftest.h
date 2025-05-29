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

#ifndef __CPU_DIFFTEST_H__
#define __CPU_DIFFTEST_H__

#include <common.h>
#include <difftest.h>
#include <memory/store_queue_wrapper.h>
#include "../memory/paddr.h"
#ifdef CONFIG_DIFFTEST
void difftest_skip_ref();
void difftest_skip_dut(int nr_ref, int nr_dut);
void difftest_set_patch(void (*fn)(void *arg), void *arg);
void difftest_step(vaddr_t pc, vaddr_t npc);
void difftest_detach();
void difftest_attach();
#else
static inline void difftest_skip_ref() {}
static inline void difftest_skip_dut(int nr_ref, int nr_dut) {}
static inline void difftest_set_patch(void (*fn)(void *arg), void *arg) {}
static inline void difftest_step(vaddr_t pc, vaddr_t npc) {}
static inline void difftest_detach() {}
static inline void difftest_attach() {}
#endif

extern void (*ref_difftest_memcpy)(paddr_t dest, void *src, size_t n, bool direction);
extern void (*ref_difftest_regcpy)(void *c, bool direction);
extern void (*ref_difftest_exec)(uint64_t n);
extern void (*ref_difftest_raise_intr)(uint64_t NO);
extern void (*ref_difftest_dirty_fsvs)(const uint64_t dirties);
extern void (*ref_difftest_pmpcpy)(void *dut, bool direction);
extern void (*ref_difftest_pmp_cfg_cpy)(void *dut, bool direction);

#ifdef CONFIG_DIFFTEST_STORE_COMMIT
extern int  (*ref_difftest_store_commit)(uint64_t *addr, uint64_t *data, uint8_t *mask);
#ifdef CONFIG_RVMATRIX
extern int  (*ref_difftest_matrix_store_commit)(uint64_t *base, uint64_t *stride, uint32_t *row, uint32_t *column, uint32_t *width, bool *transpose);
#endif // CONFIG_RVMATRIX
#endif
static inline bool difftest_check_reg(const char *name, vaddr_t pc, rtlreg_t ref, rtlreg_t dut) {
  if (ref != dut) {
    Log("%s is different after executing instruction at pc = " FMT_WORD
        ", right = " FMT_WORD ", wrong = " FMT_WORD, name, pc, ref, dut);
    return false;
  }
  return true;
}
static inline bool difftest_check_vreg(const char *name, vaddr_t pc, rtlreg_t *ref, rtlreg_t *dut,size_t n) {
  /***************ONLY FOR VLEN=128,ELEN=64**********************/
  if (memcmp(ref, dut, n)) {
    Log("%s is different after executing instruction at pc = " FMT_WORD
        ", right =  0x%016lx_%016lx , wrong =  %016lx_%016lx", name, pc, ref[1], ref[0], dut[1], dut[0]);
    return false;
  }
  return true;
}
#ifdef CONFIG_DIFFTEST_STORE_COMMIT
static inline bool difftest_check_store(vaddr_t pc) {
#ifdef CONFIG_RVV
  size_t step = store_queue_size();
  for (int i = 0; i < step ;i ++) {
#endif
    if (store_queue_empty()) return true;
    store_commit_t dut = store_queue_front();
    store_queue_pop();

    uint64_t dut_data = dut.data;
    uint64_t dut_addr = dut.addr;
    uint8_t  dut_mask = dut.mask;

    if (ref_difftest_store_commit(&dut.addr, &dut.data, &dut.mask)) {
      Log("\n\t,is different memory executing instruction at pc = " FMT_WORD,pc);
      Log(",ref addr = " FMT_WORD ", data = " FMT_WORD ", mask = 0x%x" "\n\t dut addr = " FMT_WORD ", data = " FMT_WORD ", mask = 0x%x"
          ,dut.addr, dut.data, dut.mask, dut_addr, dut_data, dut_mask);
      return false;
    }
#ifdef CONFIG_RVV
  }
#endif
  return true;
}

#if defined(CONFIG_RVMATRIX) && defined(CONFIG_DIFFTEST_STORE_COMMIT)
static inline bool difftest_check_matrix_store(vaddr_t pc) {
  if (matrix_store_queue_empty()) return true;
  matrix_store_commit_t dut = matrix_store_queue_front();
  matrix_store_queue_pop();

  uint64_t dut_base = dut.base;
  uint64_t dut_stride = dut.stride;
  uint32_t dut_row = dut.row;
  uint32_t dut_column = dut.column;
  uint32_t dut_msew = dut.msew;
  bool dut_transpose = dut.transpose;

  if (ref_difftest_matrix_store_commit(&dut.base, &dut.stride,
                                       &dut.row, &dut.column, &dut.msew, &dut.transpose)) {
    Log("\n\t,is different matrix memory executing instruction at pc = " FMT_WORD,pc);
    Log(",ref base = " FMT_WORD ", stride = " FMT_WORD ", row = " "0x%08x" ", column = " "0x%08x" ", msew = " "0x%08x" ", transpose = %d\n\t "
        "dut base = " FMT_WORD ", stride = " FMT_WORD ", row = " "0x%08x" ", column = " "0x%08x" ", msew = " "0x%08x" ", transpose = %d"
        , dut.base, dut.stride, dut.row, dut.column, dut.msew, dut.transpose,
        dut_base, dut_stride, dut_row, dut_column, dut_msew, dut_transpose);
    return false;
  }

  return true;
}
#endif // CONFIG_RVMATRIX

#endif // CONFIG_DIFFTEST_STORE_COMMIT
#endif
