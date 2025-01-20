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


static void paddr_check(paddr_t paddr) {
  if (is_in_mmio(paddr)) {
    cpu.trapInfo.tval = paddr;
    cpu.amo = false;
    longjmp_exception(EX_SAF);
  }
}

static paddr_t translate_and_check(vaddr_t vaddr) {
  Logm("Checking mmu when MMU_DYN");
  isa_mmu_check(vaddr, 8, MEM_TYPE_WRITE);
  return isa_mmu_translate(vaddr, 8, MEM_TYPE_WRITE);
}


void cbo_zero(Decode *s){
  rtlreg_t* addr_p = dsrc1;

  paddr_check(*addr_p);

  rtlreg_t  block_addr = *addr_p & ~CACHE_BLOCK_MASK;
  for (uint64_t i = 0; i < CACHE_BLOCK_OPS; i++) {
    // write zero to block_addr
    rtl_sm(s, rz, &block_addr, 0, CACHE_OP_SPLIT_SIZE, MMU_DIRECT);
    block_addr += CACHE_OP_SPLIT_SIZE;
  }
}

void cbo_inval(Decode *s){
  rtlreg_t* addr_p = dsrc1;

  paddr_check(*addr_p);
  // do nothing
  IFNDEF(CONFIG_DIFFTEST_REF_NEMU, difftest_skip_dut(1, 2));
}
void cbo_flush(Decode *s){
  rtlreg_t* addr_p = dsrc1;

  paddr_check(*addr_p);
  // do nothing
  IFNDEF(CONFIG_DIFFTEST_REF_NEMU, difftest_skip_dut(1, 2));
}

void cbo_clean(Decode *s){
  rtlreg_t* addr_p = dsrc1;

  paddr_check(*addr_p);
  // do nothing
  IFNDEF(CONFIG_DIFFTEST_REF_NEMU, difftest_skip_dut(1, 2));
}

/*
 * MMU impl
 */
void cbo_zero_mmu(Decode *s){
  rtlreg_t* addr_p = dsrc1;

  paddr_t base_addr_p = translate_and_check(*addr_p);
  paddr_check(base_addr_p);

  rtlreg_t  block_addr = *addr_p & ~CACHE_BLOCK_MASK;
  for (uint64_t i = 0; i < CACHE_BLOCK_OPS; i++) {
    // write zero to block_addr
    rtl_sm(s, rz, &block_addr, 0, CACHE_OP_SPLIT_SIZE, MMU_TRANSLATE);
    block_addr += CACHE_OP_SPLIT_SIZE;
  }
}

void cbo_inval_mmu(Decode *s){
  rtlreg_t* addr_p = dsrc1;

  paddr_t base_addr_p = translate_and_check(*addr_p);
  paddr_check(base_addr_p);
  // do nothing
  IFNDEF(CONFIG_DIFFTEST_REF_NEMU, difftest_skip_dut(1, 2));
}

void cbo_flush_mmu(Decode *s){
  rtlreg_t* addr_p = dsrc1;

  paddr_t base_addr_p = translate_and_check(*addr_p);
  paddr_check(base_addr_p);
  // do nothing
  IFNDEF(CONFIG_DIFFTEST_REF_NEMU, difftest_skip_dut(1, 2));
}

void cbo_clean_mmu(Decode *s){
  rtlreg_t* addr_p = dsrc1;

  paddr_t base_addr_p = translate_and_check(*addr_p);
  paddr_check(base_addr_p);
  // do nothing
  IFNDEF(CONFIG_DIFFTEST_REF_NEMU, difftest_skip_dut(1, 2));
}