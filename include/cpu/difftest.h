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
#ifdef CONFIG_DIFFTEST_STORE_COMMIT
extern int  (*ref_difftest_store_commit)(uint64_t *addr, uint64_t *data, uint8_t *mask);
extern void store_commit_queue_push(uint64_t addr, uint64_t data, int len);
extern store_commit_t *store_commit_queue_pop();
extern int check_store_commit(uint64_t *addr, uint64_t *data, uint8_t *mask);
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
  store_commit_t *dut =  store_commit_queue_pop();
  if (dut == NULL) return true;
  uint64_t dut_data = dut->data;
  uint64_t dut_addr = dut->addr;

  if (ref_difftest_store_commit(&dut->addr, &dut->data, &dut->mask)) {
    Log("\n\t,is different memory executing instruction at pc = " FMT_WORD,pc);
    Log(",ref addr = " FMT_WORD ", data = " FMT_WORD "\n\t dut addr = " FMT_WORD ", data = " FMT_WORD 
        ,dut->addr, dut->data, dut_addr, dut_data);  
    return false;
  }
  return true;
}
#endif // CONFIG_DIFFTEST_STORE_COMMIT
#endif
