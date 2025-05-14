/***************************************************************************************
* Copyright (c) 2020-2025 Institute of Computing Technology, Chinese Academy of Sciences
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

#include "cbo_impl.h"
#include "../local-include/intr.h"
#include "../local-include/trigger.h"
#include <common.h>
#include <rtl/rtl.h>
#include <device/mmio.h>
#include <cpu/decode.h>
#include <cpu/difftest.h>
#include <cpu/cpu.h>

#define CACHE_BLOCK_SHIFT        6
#define CACHE_BLOCK_SIZE         (1UL << CACHE_BLOCK_SHIFT)
#define CACHE_BLOCK_MASK         (CACHE_BLOCK_SIZE - 1)

#define CACHE_OP_SPLIT_SIZE      8
#define CACHE_BLOCK_OPS          (CACHE_BLOCK_SIZE / CACHE_OP_SPLIT_SIZE)


static void paddr_check(paddr_t paddr, vaddr_t vaddr, int type) {
  check_paddr(paddr, 8, type, MEM_TYPE_WRITE, cpu.mode, vaddr);
  if (!in_pmem(paddr)) {
    cpu.trapInfo.tval = vaddr;
    cpu.amo = false;
    longjmp_exception(EX_SAF);
  }
}

static void trigger_check(vaddr_t vaddr){
#ifdef CONFIG_TDATA1_MCONTROL6
  vaddr_t block_addr = vaddr & ~CACHE_BLOCK_MASK;
  for (uint64_t offset = 0; offset < CACHE_BLOCK_SIZE; offset++) {
    trig_action_t action = check_triggers_mcontrol6(cpu.TM, TRIG_OP_STORE, block_addr + offset, TRIGGER_NO_VALUE);
    trigger_handler(TRIG_TYPE_MCONTROL6, action, vaddr);
  }
#endif // CONFIG_TDATA1_MCONTROL6
}

static paddr_t translate_and_check(vaddr_t vaddr, int type) {
  Logm("Checking mmu when MMU_DYN");
  trigger_check(vaddr);
  int mmu_ret = isa_mmu_check(vaddr, 8, type);
  paddr_t paddr = vaddr;
  if (mmu_ret == MMU_TRANSLATE) {
    paddr_t pg_base = isa_mmu_translate(vaddr, 8, type);
    int ret = pg_base & PAGE_MASK;
    if (ret == MEM_RET_OK) {
      paddr = pg_base | (vaddr & PAGE_MASK);
    }
  }
  paddr_check(paddr, vaddr, type);
  return paddr;
}

static void not_translate_check(vaddr_t vaddr, int type) {
  trigger_check(vaddr);
  paddr_check(vaddr, vaddr, type);
}


static int convert_load_to_store_exception(int ori_cause) {
  switch (ori_cause) {
    case EX_LAF: case EX_LPF: case EX_LGPF: return  ori_cause + 2;
    default: return ori_cause;
  }
}


void cbo_zero(Decode *s){
  rtlreg_t* addr_p = dsrc1;

  not_translate_check(*addr_p, MEM_TYPE_WRITE);

  rtlreg_t block_addr = *addr_p & ~CACHE_BLOCK_MASK;
  for (uint64_t i = 0; i < CACHE_BLOCK_OPS; i++) {
    // write zero to block_addr
    rtl_sm(s, rz, &block_addr, 0, CACHE_OP_SPLIT_SIZE, MMU_DIRECT);
    block_addr += CACHE_OP_SPLIT_SIZE;
  }
}

void cbo_inval(Decode *s){
  int cause;
  PUSH_CONTEXT(&cause);
  if (cause) {
    pop_context();
    longjmp_exception(convert_load_to_store_exception(cause));
  } else {
    rtlreg_t* addr_p = dsrc1;

    not_translate_check(*addr_p, MEM_TYPE_READ);

    // do nothing
    IFNDEF(CONFIG_DIFFTEST_REF_NEMU, difftest_skip_dut(1, 2));
    pop_context();
  }
}

void cbo_flush(Decode *s){
  int cause;
  PUSH_CONTEXT(&cause);
  if (cause) {
    pop_context();
    longjmp_exception(convert_load_to_store_exception(cause));
  } else {
    rtlreg_t *addr_p = dsrc1;

    not_translate_check(*addr_p, MEM_TYPE_READ);

    // do nothing
    IFNDEF(CONFIG_DIFFTEST_REF_NEMU, difftest_skip_dut(1, 2));
    pop_context();
  }
}

void cbo_clean(Decode *s){
  int cause;
  PUSH_CONTEXT(&cause);
  if (cause) {
    pop_context();
    longjmp_exception(convert_load_to_store_exception(cause));
  } else {
    rtlreg_t *addr_p = dsrc1;

    not_translate_check(*addr_p, MEM_TYPE_READ);

    // do nothing
    IFNDEF(CONFIG_DIFFTEST_REF_NEMU, difftest_skip_dut(1, 2));
    pop_context();
  }
}

/*
 * MMU impl
 */
void cbo_zero_mmu(Decode *s){
  rtlreg_t *addr_p = dsrc1;

  paddr_t base_addr_p = translate_and_check(*addr_p, MEM_TYPE_WRITE);

  rtlreg_t block_addr = base_addr_p & ~CACHE_BLOCK_MASK;
  for (uint64_t i = 0; i < CACHE_BLOCK_OPS; i++) {
    // write zero to block_addr
    rtl_sm(s, rz, &block_addr, 0, CACHE_OP_SPLIT_SIZE, MMU_DIRECT);
    block_addr += CACHE_OP_SPLIT_SIZE;
  }
}

void cbo_inval_mmu(Decode *s){
  int cause;
  PUSH_CONTEXT(&cause);
  if (cause) {
    pop_context();
    longjmp_exception(convert_load_to_store_exception(cause));
  } else {

    rtlreg_t *addr_p = dsrc1;

    translate_and_check(*addr_p, MEM_TYPE_READ);

    // do nothing
    IFNDEF(CONFIG_DIFFTEST_REF_NEMU, difftest_skip_dut(1, 2));
    pop_context();
  }
}

void cbo_flush_mmu(Decode *s){
  int cause;
  PUSH_CONTEXT(&cause);
  if (cause) {
    pop_context();
    longjmp_exception(convert_load_to_store_exception(cause));
  } else {
    rtlreg_t *addr_p = dsrc1;

    translate_and_check(*addr_p, MEM_TYPE_READ);

    // do nothing
    IFNDEF(CONFIG_DIFFTEST_REF_NEMU, difftest_skip_dut(1, 2));
    pop_context();
  }
}

void cbo_clean_mmu(Decode *s){
  int cause;
  PUSH_CONTEXT(&cause);
  if (cause) {
    pop_context();
    longjmp_exception(convert_load_to_store_exception(cause));
  } else {

    rtlreg_t *addr_p = dsrc1;

    translate_and_check(*addr_p, MEM_TYPE_READ);

    // do nothing
    IFNDEF(CONFIG_DIFFTEST_REF_NEMU, difftest_skip_dut(1, 2));
    pop_context();
  }
}