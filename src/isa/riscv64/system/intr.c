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
#include "../local-include/trigger.h"
#include "../local-include/csr.h"
#include "../local-include/intr.h"
#include "../local-include/aia.h"

void update_mmu_state();


#ifdef CONFIG_RVH
word_t gen_gva(word_t NO, bool is_hls, bool is_mem_access_virtual) {
  return ((NO == EX_IAM || NO == EX_IAF || NO == EX_BP || NO == EX_IPF) && cpu.v) ||
         ((NO == EX_LAM || NO == EX_LAF || NO == EX_SAM || NO == EX_SAF || NO == EX_LPF || NO == EX_SPF) && (is_hls || cpu.v || is_mem_access_virtual)) ||
         (NO == EX_IGPF || NO == EX_LGPF || NO == EX_SGPF);
}

bool intr_deleg_S(word_t exceptionNO) {
  bool isNMI = MUXDEF(CONFIG_RV_SMRNMI, cpu.hasNMI && (exceptionNO & INTR_BIT), false);
  word_t deleg = (exceptionNO & INTR_BIT ? mideleg->val : medeleg->val);
  bool delegS = ((deleg & (1 << (exceptionNO & 0xff))) != 0) && (cpu.mode < MODE_M) && !isNMI;
  return delegS;
}
bool intr_deleg_VS(word_t exceptionNO){
  bool isNMI = MUXDEF(CONFIG_RV_SMRNMI, cpu.hasNMI && (exceptionNO & INTR_BIT), false);
  bool delegS = intr_deleg_S(exceptionNO);
  word_t deleg = (exceptionNO & INTR_BIT ? get_hideleg() : hedeleg->val);
  bool delegVS = cpu.v && ((deleg & (1 << (exceptionNO & 0xff))) != 0) && (cpu.mode < MODE_M) && !isNMI;
  return delegS && delegVS;
}

#else
bool intr_deleg_S(word_t exceptionNO) {
  bool isNMI = MUXDEF(CONFIG_RV_SMRNMI, cpu.hasNMI && (exceptionNO & INTR_BIT), false);
  word_t deleg = (exceptionNO & INTR_BIT ? mideleg->val : medeleg->val);
  bool delegS = ((deleg & (1 << (exceptionNO & 0xf))) != 0) && (cpu.mode < MODE_M) && !isNMI;
  return delegS;
}
#endif

void clear_trapinfo(){
  memset(&cpu.trapInfo, 0, sizeof(trap_info_t));
}

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
  Logti("raise intr cause NO: %ld, epc: %lx\n", NO, epc);
#ifdef CONFIG_DIFFTEST_REF_SPIKE
  switch (NO) {
    // ecall and ebreak are handled normally
#ifdef CONFIG_RVH
    // case EX_ECVS:
    case EX_IGPF:
    case EX_LGPF:
    case EX_VI:
    case EX_SGPF:
#endif
    case EX_IAM:
    case EX_IAF:
    case EX_II:
    // case EX_BP:
    case EX_LAM:
    case EX_LAF:
    case EX_SAM:
    case EX_SAF:
    // case EX_ECU:
    // case EX_ECS:
    // case EX_ECM:
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
#ifdef CONFIG_RV_SMRNMI
  if (!mnstatus->nmie){
#ifdef CONFIG_SHARE
    IFDEF(CONFIG_RV_SMDBLTRP, cpu.critical_error = true);// this will compare in difftest
#else
    printf("\33[1;31mHIT CRITICAL ERROR\33[0m: trap when mnstatus.nmie close, please check if software cause a double trap.\n");
    nemu_state.state = NEMU_END;
    nemu_state.halt_pc = epc;
    nemu_state.halt_ret = 0;
#endif // CONFIG_SHARE
    return 0;
  }
#endif // CONFIG_RV_SMRNMI
  bool isNMI = MUXDEF(CONFIG_RV_SMRNMI, cpu.hasNMI && (NO & INTR_BIT), false);
  bool delegS = intr_deleg_S(NO);
  bool delegM = !delegS && !isNMI;
  bool s_EX_DT = MUXDEF(CONFIG_RV_SSDBLTRP, delegS && mstatus->sdt, false);
  bool m_EX_DT = MUXDEF(CONFIG_RV_SMDBLTRP, delegM && mstatus->mdt, false);
  word_t trap_pc = 0;
#ifdef CONFIG_RVH
  bool virtualInterruptIsHvictlInject = MUXDEF(CONFIG_RV_IMSIC, cpu.virtualInterruptIsHvictlInject, false);
  extern bool hld_st;
  int hld_st_temp = hld_st;
  hld_st = 0;
  bool delegVS = intr_deleg_VS(NO);
  delegM = !delegS && !delegVS && !isNMI;
  delegS &= !delegVS;
#ifdef CONFIG_RV_IMSIC
  if (NO & INTR_BIT) {
    delegS  = cpu.interrupt_delegate.interrupt_to_hs;
    delegVS = cpu.interrupt_delegate.interrupt_to_vs;
    delegM = !delegS && !delegVS && !isNMI;
  }
#endif
  bool vs_EX_DT = MUXDEF(CONFIG_RV_SSDBLTRP, delegVS && vsstatus->sdt, false);
  m_EX_DT = MUXDEF(CONFIG_RV_SMDBLTRP, delegM && mstatus->mdt, false);
  if ((delegVS && !vs_EX_DT) || (virtualInterruptIsHvictlInject && !isNMI)){
#ifdef CONFIG_RV_IMSIC
    int vs_guest_no = NO & 0xff;
    bool vs_host_no = (vs_guest_no == IRQ_VSSIP) || (vs_guest_no == IRQ_VSTIP) || (vs_guest_no == IRQ_VSEIP);
    if ((NO & INTR_BIT) && vs_host_no) {
      vscause->val = ((NO & (~INTR_BIT)) - 1) | INTR_BIT;
    } else {
      vscause->val = NO;
    }
#else
    if (virtualInterruptIsHvictlInject) {
      vscause->val = NO | INTR_BIT;
#ifdef CONFIG_RV_IMSIC
      cpu.virtualInterruptIsHvictlInject = 0;
#endif
    } else {
      vscause->val = NO & INTR_BIT ? ((NO & (~INTR_BIT)) - 1) | INTR_BIT : NO;
    }
#endif // CONFIG_RV_IMSIC
    vsepc->val = epc;
    vsstatus->spp = cpu.mode;
    vsstatus->spie = vsstatus->sie;
    vsstatus->sie = 0;
    vsstatus->sdt = MUXDEF(CONFIG_RV_SSDBLTRP, henvcfg->dte && menvcfg->dte, 0);
    vstval->val = cpu.trapInfo.tval;
    switch (NO) {
      case EX_IPF: case EX_LPF: case EX_SPF:
      case EX_LAM: case EX_SAM:
      case EX_IAF: case EX_LAF: case EX_SAF:
        break;
      case EX_BP :
#ifdef CONFIG_RV_SDTRIG
        switch (trigger_action) {
          case TRIG_ACTION_NONE: vstval->val = epc; break;
          case TRIG_ACTION_BKPT_EXCPT:
            vstval->val = triggered_tval;
            trigger_action = TRIG_ACTION_NONE;
            break;
          default: panic("Unsupported trigger action %d", trigger_action);  break;
        }
#else
        vstval->val = epc;
#endif // CONFIG_RV_SDTRIG
        break;
      case EX_II:
        vstval->val = MUXDEF(CONFIG_TVAL_EX_II, cpu.instr, 0);
        break;
      default: vstval->val = 0;
    }
    cpu.v = 1;
    cpu.mode = MODE_S;
    trap_pc = get_trap_pc(vstvec->val, vscause->val);
  }
  else if(delegS && !s_EX_DT){
    hstatus->gva = gen_gva(NO, hld_st_temp, false);
    hstatus->spv = cpu.v;
    if(cpu.v){
      hstatus->spvp = cpu.mode;
    }
    cpu.v = 0;
#else
  bool vs_EX_DT = false;
  if (delegS && !s_EX_DT) {
#endif
    scause->val = NO;
    sepc->val = epc;
    mstatus->spp = cpu.mode;
    mstatus->spie = mstatus->sie;
    mstatus->sie = 0;
    mstatus->sdt = MUXDEF(CONFIG_RV_SSDBLTRP, menvcfg->dte, 0);
    IFDEF(CONFIG_RVH, htval->val = cpu.trapInfo.tval2);
    IFDEF(CONFIG_RVH, htinst->val = cpu.trapInfo.tinst);
    stval->val = cpu.trapInfo.tval;
    switch (NO) {
      case EX_IPF: case EX_LPF: case EX_SPF:
      case EX_LAM: case EX_SAM:
      case EX_IAF: case EX_LAF: case EX_SAF:
        IFDEF(CONFIG_RVH, htval->val = 0);
        IFDEF(CONFIG_RVH, htinst->val = 0);
        break;
      case EX_IGPF: case EX_LGPF: case EX_SGPF:
        break;
      case EX_II: case EX_VI:
        stval->val = MUXDEF(CONFIG_TVAL_EX_II, cpu.instr, 0);
        IFDEF(CONFIG_RVH, htval->val = 0);
        IFDEF(CONFIG_RVH, htinst->val = 0);
        break;
      case EX_BP :
#ifdef CONFIG_RV_SDTRIG
        switch (trigger_action) {
          case TRIG_ACTION_NONE: stval->val = epc; break;
          case TRIG_ACTION_BKPT_EXCPT:
            stval->val = triggered_tval;
            trigger_action = TRIG_ACTION_NONE;
            break;
          default: panic("Unsupported trigger action %d", trigger_action);  break;
        }
#else
        stval->val = epc;
#endif // CONFIG_RV_SDTRIG
        IFDEF(CONFIG_RVH, htval->val = 0);
        IFDEF(CONFIG_RVH, htinst->val = 0);
        break;
      default:
        stval->val = 0;
        IFDEF(CONFIG_RVH, htval->val = 0);
        IFDEF(CONFIG_RVH, htinst->val = 0);
    }
    // When a trap (except GPF) is taken into HS-mode, htinst is written with 0.
    // Todo: support tinst encoding descriped in section
    // 18.6.3. Transformed Instruction or Pseudoinstruction for mtinst or htinst.
    cpu.mode = MODE_S;
    trap_pc = get_trap_pc(stvec->val, scause->val);
  } else if((delegM || vs_EX_DT || s_EX_DT) && !m_EX_DT){
#ifdef CONFIG_RVH
    bool is_mem_access_virtual = mstatus->mprv && mstatus->mpv && (mstatus->mpp != MODE_M);
    mstatus->gva = gen_gva(NO, hld_st_temp, is_mem_access_virtual);
    mstatus->mpv = cpu.v;
    cpu.v = 0;
#endif
    mcause->val = NO;
    mepc->val = epc;
    mstatus->mpp = cpu.mode;
    mstatus->mpie = mstatus->mie;
    mstatus->mie = 0;
    mtval->val = cpu.trapInfo.tval;
    IFDEF(CONFIG_RVH, mtval2->val = cpu.trapInfo.tval2);
    IFDEF(CONFIG_RVH, mtinst->val = cpu.trapInfo.tinst);
    switch (NO) {
      case EX_IPF: case EX_LPF: case EX_SPF:
      case EX_LAM: case EX_SAM:
      case EX_IAF: case EX_LAF: case EX_SAF:
        IFDEF(CONFIG_RVH, mtval2->val = 0);
        IFDEF(CONFIG_RVH, mtinst->val = 0);
        break;
      case EX_IGPF: case EX_LGPF: case EX_SGPF:
        break;
      case EX_II: case EX_VI:
        mtval->val = MUXDEF(CONFIG_TVAL_EX_II, cpu.instr, 0);
        IFDEF(CONFIG_RVH, mtval2->val = 0);
        IFDEF(CONFIG_RVH, mtinst->val = 0);
        break;
      case EX_BP:
#ifdef CONFIG_RV_SDTRIG
        switch (trigger_action) {
          case TRIG_ACTION_NONE: mtval->val = epc; break;
          case TRIG_ACTION_BKPT_EXCPT:
            mtval->val = triggered_tval;
            trigger_action = TRIG_ACTION_NONE;
            break;
          default: panic("Unsupported trigger action %d", trigger_action);  break;
        }
#else
        mtval->val = epc;
#endif // CONFIG_RV_SDTRIG
        IFDEF(CONFIG_RVH, mtval2->val = 0);
        IFDEF(CONFIG_RVH, mtinst->val = 0);
        break;
      default:
        mtval->val = 0;
        IFDEF(CONFIG_RVH, mtval2->val = 0);
        IFDEF(CONFIG_RVH, mtinst->val = 0);
    }
#ifdef CONFIG_RV_SSDBLTRP
    bool hasEX_DT = vs_EX_DT || s_EX_DT;
    mstatus->mdt = 1;
    mcause->val = (hasEX_DT ? EX_DT : mcause->val);
    mtval2->val = (hasEX_DT ? NO : mtval2->val);
#endif //CONFIG_RV_SSDBLTRP
    cpu.mode = MODE_M;
    trap_pc = get_trap_pc(mtvec->val, mcause->val);
  }
#ifdef CONFIG_RV_SMRNMI
  else if ((m_EX_DT || isNMI) && mnstatus->nmie) {
    mnstatus->mnpp = cpu.mode;
#ifdef CONFIG_RVH
    mnstatus->mnpv = cpu.v;
#endif //CONFIG_RVH
    mnstatus->nmie = 0;
    mnepc->val = epc;
    mncause->val = NO;
    cpu.mode = MODE_M;
    trap_pc = get_trap_pc(mtvec->val, mncause->val);
  }
#endif //CONFIG_RV_SMRNMI
  update_mmu_state();
  clear_trapinfo();
  return trap_pc;
}

word_t isa_query_intr() {
  word_t intr_vec = mie->val & (get_mip());
  if (!intr_vec || MUXDEF(CONFIG_RV_SMRNMI,!mnstatus->nmie, false)) return INTR_EMPTY;
  int intr_num;
#ifdef CONFIG_RVH
  const int priority [] = {
    IRQ_MEIP, IRQ_MSIP, IRQ_MTIP,
    IRQ_SEIP, IRQ_SSIP, IRQ_STIP,
    IRQ_UEIP, IRQ_USIP, IRQ_UTIP,
    IRQ_SGEI,
    IRQ_VSEIP, IRQ_VSSIP, IRQ_VSTIP,
#ifdef CONFIG_RV_SSCOFPMF
    IRQ_LCOFI
#endif
  };
#ifdef CONFIG_RV_SSCOFPMF
  intr_num = 14;
#else
  intr_num = 13;
#endif
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
      bool hdeleg = (get_hideleg() & (1 << irq)) != 0;
      bool global_enable = (hdeleg & deleg)? (cpu.v && cpu.mode == MODE_S && vsstatus->sie) || (cpu.v && cpu.mode < MODE_S):
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
