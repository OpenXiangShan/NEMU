/***************************************************************************************
* Copyright (c) 2020-2024 Institute of Computing Technology, Chinese Academy of Sciences
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

#include <common.h>

#define CACHE_BLOCK_SHIFT        6
#define CACHE_BLOCK_SIZE         (1UL << CACHE_BLOCK_SHIFT)
#define CACHE_BLOCK_MASK         (CACHE_BLOCK_SIZE - 1)

#define CACHE_OP_SPLIT_SIZE      8
#define CACHE_BLOCK_OPS          (CACHE_BLOCK_SIZE / CACHE_OP_SPLIT_SIZE)

def_EHelper(cbo_zero) {
  // check illegal instruction exception
  if((cpu.mode != MODE_M && !menvcfg->cbze) || (cpu.mode == MODE_U && !senvcfg->cbze)){
    longjmp_exception(EX_II);
  } else if(cpu.v && ((cpu.mode == MODE_S && !henvcfg->cbze) || (cpu.mode == MODE_U && !(henvcfg->cbze && senvcfg->cbze)))){
    longjmp_exception(EX_VI);
  }

  rtlreg_t* addr_p = dsrc1;
  rtlreg_t  block_addr = *addr_p & ~CACHE_BLOCK_MASK;
  for (int i = 0; i < CACHE_BLOCK_OPS; i++) {
    // write zero to block_addr
    rtl_sm(s, rz, &block_addr, 0, CACHE_OP_SPLIT_SIZE, MMU_DIRECT);
    block_addr += CACHE_OP_SPLIT_SIZE;
  }
}

def_EHelper(cbo_zero_mmu) {
  // check illegal instruction exception
  if((cpu.mode != MODE_M && !menvcfg->cbze) || (cpu.mode == MODE_U && !senvcfg->cbze)){
    longjmp_exception(EX_II);
  } else if(cpu.v && ((cpu.mode == MODE_S && !henvcfg->cbze) || (cpu.mode == MODE_U && !(henvcfg->cbze && senvcfg->cbze)))){
    longjmp_exception(EX_VI);
  }
  
  rtlreg_t* addr_p = dsrc1;
  rtlreg_t  block_addr = *addr_p & ~CACHE_BLOCK_MASK;
  for (int i = 0; i < CACHE_BLOCK_OPS; i++) {
    // write zero to block_addr
    rtl_sm(s, rz, &block_addr, 0, CACHE_OP_SPLIT_SIZE, MMU_TRANSLATE);
    block_addr += CACHE_OP_SPLIT_SIZE;
  }
}

def_EHelper(cbo_inval) {
  // check illegal instruction exception
  if((cpu.mode != MODE_M && menvcfg->cbie == 0) || (cpu.mode == MODE_U && senvcfg->cbie == 0)){
    longjmp_exception(EX_II);
  } else if(cpu.v && ((cpu.mode == MODE_S && henvcfg->cbie == 0) || (cpu.mode == MODE_U && (henvcfg->cbie == 0 || senvcfg->cbie == 0)))){
    longjmp_exception(EX_VI);
  }
  // do nothing
  IFNDEF(CONFIG_DIFFTEST_REF_NEMU, difftest_skip_dut(1, 2));
}

def_EHelper(cbo_flush) {
  // check illegal instruction exception
  if((cpu.mode != MODE_M && !menvcfg->cbcfe) || (cpu.mode == MODE_U && !senvcfg->cbcfe)){
    longjmp_exception(EX_II);
  } else if(cpu.v && ((cpu.mode == MODE_S && !henvcfg->cbcfe) || (cpu.mode == MODE_U && !(henvcfg->cbcfe && senvcfg->cbcfe)))){
    longjmp_exception(EX_VI);
  }
  // do nothing
  IFNDEF(CONFIG_DIFFTEST_REF_NEMU, difftest_skip_dut(1, 2));
}

def_EHelper(cbo_clean) {
  // check illegal instruction exception
  if((cpu.mode != MODE_M && !menvcfg->cbcfe) || (cpu.mode == MODE_U && !senvcfg->cbcfe)){
    longjmp_exception(EX_II);
  } else if(cpu.v && ((cpu.mode == MODE_S && !henvcfg->cbcfe) || (cpu.mode == MODE_U && !(henvcfg->cbcfe && senvcfg->cbcfe)))){
    longjmp_exception(EX_VI);
  }
  // do nothing
  IFNDEF(CONFIG_DIFFTEST_REF_NEMU, difftest_skip_dut(1, 2));
}
