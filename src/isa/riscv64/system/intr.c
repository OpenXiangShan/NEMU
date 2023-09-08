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
#include <cpu/cpu.h>
#include "../local-include/csr.h"
#include "../local-include/intr.h"

void update_mmu_state();

#define INTR_BIT (1ULL << 63)
enum {
  IRQ_USIP, IRQ_SSIP, IRQ_VSSIP, IRQ_MSIP,
  IRQ_UTIP, IRQ_STIP, IRQ_VSTIP, IRQ_MTIP,
  IRQ_UEIP, IRQ_SEIP, IRQ_VSEIP, IRQ_MEIP, IRQ_SGEI
};

#ifdef CONFIG_RVH
bool intr_deleg_S(word_t exceptionNO) {
  word_t deleg = (exceptionNO & INTR_BIT ? mideleg->val : medeleg->val);
  bool delegS = ((deleg & (1 << (exceptionNO & 0xff))) != 0) && (cpu.mode < MODE_M);
  return delegS;
}
bool intr_deleg_VS(word_t exceptionNO){
  bool delegS = intr_deleg_S(exceptionNO);
  word_t deleg = (exceptionNO & INTR_BIT ? hideleg->val : hedeleg->val);
  bool delegVS = cpu.v && ((deleg & (1 << (exceptionNO & 0xff))) != 0) && (cpu.mode < MODE_M);
  return delegS && delegVS;
}

#else
bool intr_deleg_S(word_t exceptionNO) {
  word_t deleg = (exceptionNO & INTR_BIT ? mideleg->val : medeleg->val);
  bool delegS = ((deleg & (1 << (exceptionNO & 0xf))) != 0) && (cpu.mode < MODE_M);
  return delegS;
}
#endif

static word_t get_trap_pc(word_t xtvec, word_t xcause) {
  word_t base = (xtvec >> 2) << 2;
  word_t mode = (xtvec & 0x1); // bit 1 is reserved, dont care here.
  bool is_intr = (xcause >> (sizeof(word_t)*8 - 1)) == 1;
#ifdef CONFIG_RVH
  word_t cause_no = xcause & 0xff;
#else
  word_t cause_no = xcause & 0xf;
#endif
  return (is_intr && mode==1) ? (base + (cause_no << 2)) : base;
}

word_t raise_intr(word_t NO, vaddr_t epc) {
#ifdef CONFIG_DIFFTEST_REF_SPIKE
  switch (NO) {
#ifdef CONFIG_RVH
    case EX_VI:
    case EX_IGPF:
    case EX_LGPF:
    case EX_SGPF:
#endif
    case EX_II:
    case EX_IPF:
    case EX_LPF:
    case EX_SPF: difftest_skip_dut(1, 0); break;
  }
#else
  switch (NO) {
#ifdef CONFIG_RVH
    case EX_VI:
    case EX_IGPF:
    case EX_LGPF:
    case EX_SGPF:
#endif
    case EX_II:
    case EX_IPF:
    case EX_LPF:
    case EX_SPF: difftest_skip_dut(1, 2); break;
  }
#endif
  bool delegS = intr_deleg_S(NO);
#ifdef CONFIG_RVH
  extern bool hld_st;
  int hld_st_temp = hld_st;
  hld_st = 0;
  bool delegVS = intr_deleg_VS(NO);
  if (delegVS){
    vscause->val = NO & INTR_BIT ? ((NO & (~INTR_BIT)) - 1) | INTR_BIT : NO;
    vsepc->val = epc;
    if (hstatus->vsxl == 1){
      vsstatus->_32.spp = cpu.mode;
      vsstatus->_32.spie = vsstatus->_32.sie;
      vsstatus->_32.sie = 0;
    }else{
      vsstatus->_64.spp = cpu.mode;
      vsstatus->_64.spie = vsstatus->_64.sie;
      vsstatus->_64.sie = 0;
    }
    // vsstatus->spp = cpu.mode;
    // vsstatus->spie = vsstatus->sie;
    // vsstatus->sie = 0;
    switch (NO) {
      case EX_IPF: case EX_LPF: case EX_SPF:
      case EX_LAM: case EX_SAM:
      case EX_IAF: case EX_LAF: case EX_SAF:
        break;
      default: vstval->val = 0;
    }
    cpu.v = 1;
    cpu.mode = MODE_S;
    update_mmu_state();
    return get_trap_pc(vstvec->val, vscause->val);
  }else if(delegS){
    int v = (mstatus->mprv)? mstatus->mpv : cpu.v;
    hstatus->gva = (NO == EX_IGPF || NO == EX_LGPF || NO == EX_SGPF ||
                    ((v || hld_st_temp) && ((0 <= NO && NO <= 7 && NO != 2) || NO == EX_IPF || NO == EX_LPF || NO == EX_SPF)));
    hstatus->spv = cpu.v;
    if(cpu.v){
      hstatus->spvp = cpu.mode; 
    }
    cpu.v = 0;
    set_sys_state_flag(SYS_STATE_FLUSH_TCACHE);
#else
  if (delegS) {
#endif
    scause->val = NO;
    sepc->val = epc;
    mstatus->spp = cpu.mode;
    mstatus->spie = mstatus->sie;
    mstatus->sie = 0;
    switch (NO) {
      case EX_IPF: case EX_LPF: case EX_SPF:
      case EX_LAM: case EX_SAM:
      case EX_IAF: case EX_LAF: case EX_SAF:
#ifdef CONFIG_RVH
        htval->val = 0;
        break;
      case EX_IGPF: case EX_LGPF: case EX_SGPF:
#endif
        break;
#ifdef CONFIG_TVAL_EX_II
      case EX_II: stval->val = cpu.instr; break;
#endif
      default: stval->val = 0;
#ifdef CONFIG_RVH
               htval->val = 0;
#endif
    }
    cpu.mode = MODE_S;
    update_mmu_state();
    return get_trap_pc(stvec->val, scause->val);
    // return stvec->val;
  } else {
#ifdef CONFIG_RVH
    int v = (mstatus->mprv)? mstatus->mpv : cpu.v;
    mstatus->gva = (NO == EX_IGPF || NO == EX_LGPF || NO == EX_SGPF ||
                    ((v || hld_st_temp) && ((0 <= NO && NO <= 7 && NO != 2) || NO == EX_IPF || NO == EX_LPF || NO == EX_SPF)));
    mstatus->mpv = cpu.v;
    cpu.v = 0;set_sys_state_flag(SYS_STATE_FLUSH_TCACHE);
#endif
    mcause->val = NO;
    mepc->val = epc;
    mstatus->mpp = cpu.mode;
    mstatus->mpie = mstatus->mie;
    mstatus->mie = 0;
    switch (NO) {
      case EX_IPF: case EX_LPF: case EX_SPF:
      case EX_LAM: case EX_SAM:
      case EX_IAF: case EX_LAF: case EX_SAF:
#ifdef CONFIG_RVH
        mtval2->val = 0;
        break;
      case EX_IGPF: case EX_LGPF: case EX_SGPF:
#endif
        break;
#ifdef CONFIG_TVAL_EX_II
      case EX_II: mtval->val = cpu.instr; break;
#endif
      default: mtval->val = 0;
#ifdef CONFIG_RVH
               mtval2->val = 0;
#endif
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
  int intr_num;
#ifdef CONFIG_RVH
  const int priority [] = {
    IRQ_MEIP, IRQ_MSIP, IRQ_MTIP,
    IRQ_SEIP, IRQ_SSIP, IRQ_STIP,
    IRQ_UEIP, IRQ_USIP, IRQ_UTIP,
    IRQ_VSEIP, IRQ_VSSIP, IRQ_VSTIP, IRQ_SGEI
  };
  intr_num = 13;
#else
  const int priority [] = {
    IRQ_MEIP, IRQ_MSIP, IRQ_MTIP,
    IRQ_SEIP, IRQ_SSIP, IRQ_STIP,
    IRQ_UEIP, IRQ_USIP, IRQ_UTIP
  };
  intr_num = 9;
#endif // CONFIG_RVH
  int i;

  for (i = 0; i < intr_num; i ++) {
    int irq = priority[i];
    if (intr_vec & (1 << irq)) {
      bool deleg = (mideleg->val & (1 << irq)) != 0;
#ifdef CONFIG_RVH
      bool hdeleg = (hideleg->val & (1 << irq)) != 0;
      bool global_enable = (hdeleg & deleg)? (cpu.v && cpu.mode == MODE_S && ((hstatus->vsxl == 1)? vsstatus->_32.sie: vsstatus->_64.sie)) || (cpu.v && cpu.mode < MODE_S):
                           (deleg)? ((cpu.mode == MODE_S) && mstatus->sie) || (cpu.mode < MODE_S) || cpu.v:
                           ((cpu.mode == MODE_M) && mstatus->mie) || (cpu.mode < MODE_M);  
#else
      bool global_enable = (deleg ? ((cpu.mode == MODE_S) && mstatus->sie) || (cpu.mode < MODE_S) :
          ((cpu.mode == MODE_M) && mstatus->mie) || (cpu.mode < MODE_M));
#endif
      if (global_enable) return irq | INTR_BIT;
    }
  }
  return INTR_EMPTY;
}