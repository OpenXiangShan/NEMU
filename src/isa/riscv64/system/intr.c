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

#include <cpu/difftest.h>
#include "../local-include/csr.h"
#include "../local-include/intr.h"

void update_mmu_state();

#define INTR_BIT (1ULL << 63)
enum {
  IRQ_USIP, IRQ_SSIP, IRQ_HSIP, IRQ_MSIP,
  IRQ_UTIP, IRQ_STIP, IRQ_HTIP, IRQ_MTIP,
  IRQ_UEIP, IRQ_SEIP, IRQ_HEIP, IRQ_MEIP
};

bool intr_deleg_S(word_t exceptionNO) {
  word_t deleg = (exceptionNO & INTR_BIT ? mideleg->val : medeleg->val);
  bool delegS = ((deleg & (1 << (exceptionNO & 0xf))) != 0) && (cpu.mode < MODE_M);
  return delegS;
}

static word_t get_trap_pc(word_t xtvec, word_t xcause) {
  word_t base = (xtvec >> 2) << 2;
  word_t mode = (xtvec & 0x1); // bit 1 is reserved, dont care here.
  bool is_intr = (xcause >> (sizeof(word_t)*8 - 1)) == 1;
  word_t casue_no = xcause & 0xf;
  return (is_intr && mode==1) ? (base + (casue_no << 2)) : base;
}

word_t raise_intr(word_t NO, vaddr_t epc) {
  switch (NO) {
    case EX_II:
    case EX_IPF:
    case EX_LPF:
    case EX_SPF: difftest_skip_dut(1, 2); break;
  }

  bool delegS = intr_deleg_S(NO);

  if (delegS) {
    scause->val = NO;
    sepc->val = epc;
    mstatus->spp = cpu.mode;
    mstatus->spie = mstatus->sie;
    mstatus->sie = 0;
    switch (NO) {
      case EX_IPF: case EX_LPF: case EX_SPF:
      case EX_LAM: case EX_SAM:
      case EX_IAF: case EX_LAF: case EX_SAF:
        break;
      default: stval->val = 0;
    }
    cpu.mode = MODE_S;
    update_mmu_state();
    return get_trap_pc(stvec->val, scause->val);
    // return stvec->val;
  } else {
    mcause->val = NO;
    mepc->val = epc;
    mstatus->mpp = cpu.mode;
    mstatus->mpie = mstatus->mie;
    mstatus->mie = 0;
    switch (NO) {
      case EX_IPF: case EX_LPF: case EX_SPF:
      case EX_LAM: case EX_SAM:
      case EX_IAF: case EX_LAF: case EX_SAF:
        break;
      default: mtval->val = 0;
    }
    cpu.mode = MODE_M;
    update_mmu_state();
    return get_trap_pc(mtvec->val, mcause->val);
    // return mtvec->val;
  }
}

word_t isa_query_intr() {
  word_t intr_vec = mie->val & mip->val;
  if (!intr_vec) return INTR_EMPTY;

  const int priority [] = {
    IRQ_MEIP, IRQ_MSIP, IRQ_MTIP,
    IRQ_SEIP, IRQ_SSIP, IRQ_STIP,
    IRQ_UEIP, IRQ_USIP, IRQ_UTIP
  };
  int i;
  for (i = 0; i < 9; i ++) {
    int irq = priority[i];
    if (intr_vec & (1 << irq)) {
      bool deleg = (mideleg->val & (1 << irq)) != 0;
      bool global_enable = (deleg ? ((cpu.mode == MODE_S) && mstatus->sie) || (cpu.mode < MODE_S) :
          ((cpu.mode == MODE_M) && mstatus->mie) || (cpu.mode < MODE_M));
      if (global_enable) return irq | INTR_BIT;
    }
  }
  return INTR_EMPTY;
}