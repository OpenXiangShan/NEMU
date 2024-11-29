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

#include <cpu/cpu.h>
#include <memory/paddr.h>
#include <rtl/rtl.h>
#include "../local-include/trigger.h"
#include "../local-include/intr.h"
#include "cpu/difftest.h"
__attribute__((cold))
def_rtl(amo_slow_path, rtlreg_t *dest, const rtlreg_t *src1, const rtlreg_t *src2) {
  uint32_t funct5 = s->isa.instr.r.funct7 >> 2;
  int rd = s->isa.instr.r.rd;
  int rs2 = s->isa.instr.r.rs2;
  int width = s->isa.instr.r.funct3 & 1 ? 8 : 4;
  width = BITS(s->isa.instr.r.funct3, 2, 2) == 0 ? width : 16;

  if (funct5 == 0b00101) { // amocas
    if (width == 16 && ((rd % 2 == 1) || (rs2 % 2 == 1))) { // amocas.q 128-bit
      longjmp_exception(EX_II);
    }
  }

#ifdef CONFIG_TDATA1_MCONTROL6
  trig_action_t action = TRIG_ACTION_NONE;
  if (funct5 == 0b00010) { // lr
    action = check_triggers_mcontrol6(cpu.TM, TRIG_OP_LOAD, *src1, TRIGGER_NO_VALUE); trigger_handler(TRIG_TYPE_MCONTROL6, action, *src1);
  } else if(funct5 == 0b00011) { // sc
    action = check_triggers_mcontrol6(cpu.TM, TRIG_OP_STORE, *src1, TRIGGER_NO_VALUE); trigger_handler(TRIG_TYPE_MCONTROL6, action, *src1);
  } else { // amo
    action = check_triggers_mcontrol6(cpu.TM, TRIG_OP_LOAD, *src1, TRIGGER_NO_VALUE); trigger_handler(TRIG_TYPE_MCONTROL6, action, *src1);
    action = check_triggers_mcontrol6(cpu.TM, TRIG_OP_STORE, *src1, TRIGGER_NO_VALUE); trigger_handler(TRIG_TYPE_MCONTROL6, action, *src1);
  }
#endif // CONFIG_TDATA1_MCONTROL6

  // AMO does not support misalign operation
  // So check misalign before real memory access
  void isa_amo_misalign_data_addr_check(vaddr_t vaddr, int len, int type);
  isa_amo_misalign_data_addr_check(*src1, width, (funct5 == 0b00010) ? MEM_TYPE_READ : MEM_TYPE_WRITE);

  if (funct5 == 0b00010) { // lr
    assert(!cpu.amo);
    cpu.lr_addr = *src1;
    cpu.lr_valid = 1;
    rtl_lms(s, dest, src1, 0, width, MMU_DYNAMIC);
    Logti("set lr vaild");
    return;
  } else if (funct5 == 0b00011) { // sc
#ifdef CONFIG_DIFFTEST_STORE_COMMIT
    // cpu.amo for sc instructions is set to true when store difftest is enabled.
    // Atomic instructions don't commit through store queue and need to be skipped.
    // Store difftest uses cpu.amo to skip atomic store instructions.
    cpu.amo = true;
#endif
    // should check overlapping instead of equality
    int success = ((cpu.lr_addr ^ *src1) >> CONFIG_RESERVATION_SET_WIDTH == 0) && cpu.lr_valid;
    Logti("cpu sc addr=%lx scr1=%lx vaild=%ld success=%d", cpu.lr_addr,*src1, cpu.lr_valid,success);
    cpu.lr_valid = 0;
    if (success) {
      rtl_sm(s, src2, src1, 0, width, MMU_DYNAMIC);
    } else {
    // Because spike skipped some exception or interrupt
    // the atomic operation would fail after handling the exception
    // so we need to make spike fail as well
      IFDEF(CONFIG_DIFFTEST_REF_SPIKE,difftest_skip_ref());
      cpu.lr_valid = 0;
      // Even if scInvalid, SPF (if raised) also needs to be reported
      uint64_t paddr = *dsrc1;
      if (isa_mmu_check(*dsrc1, width, MEM_TYPE_WRITE) == MMU_TRANSLATE) {
        paddr = isa_mmu_translate(*dsrc1, width, MEM_TYPE_WRITE);
      }
      return_on_mem_ex();
      // Even if scInvalid, SAF (if raised) also needs to be reported
      // Check address space range and pmp
      if (!in_pmem(paddr) || !isa_pmp_check_permission(paddr, width, MEM_TYPE_WRITE, cpu.mode)) {
        cpu.trapInfo.tval = *src1;
        longjmp_exception(EX_SAF);
      }
    }
    rtl_li(s, dest, !success);
#ifdef CONFIG_DIFFTEST_STORE_COMMIT
    cpu.amo = false;
#endif
    return;
  }

#ifdef CONFIG_RV_ZACAS
  if (funct5 == 0b00101) { // amocas
    cpu.amo = true;
    // check store behavior before actually load or store
    uint64_t paddr = *dsrc1;
    if (isa_mmu_check(*dsrc1, width, MEM_TYPE_WRITE) == MMU_TRANSLATE) {
      paddr = isa_mmu_translate(*dsrc1, width, MEM_TYPE_WRITE);
    }
    if (!in_pmem(paddr) || !isa_pmp_check_permission(paddr, width, MEM_TYPE_WRITE, cpu.mode)) {
      cpu.trapInfo.tval = *src1;
      longjmp_exception(EX_SAF);
    }
    switch (width) {
      case 4:
        rtl_lms(s, s0, src1, 0, 4, MMU_DYNAMIC);
        if ((int32_t)*dest == (int32_t)*s0) {
          *s1 = *src2;
          rtl_sm(s, s1, src1, 0, 4, MMU_DYNAMIC);
        }
        rtl_mv(s, dest, s0);
        break;
      case 8:
        rtl_lms(s, s0, src1, 0, 8, MMU_DYNAMIC);
        if ((int64_t)*dest == (int64_t)*s0) {
          *s1 = *src2;
          rtl_sm(s, s1, src1, 0, 8, MMU_DYNAMIC);
        }
        rtl_mv(s, dest, s0);
        break;
      case 16:
        rtl_lms(s, s0, src1, 0, 8, MMU_DYNAMIC);
        rtl_lms(s, s1, src1, 8, 8, MMU_DYNAMIC);
        *t0 = rd == 0 ? 0 : *(dest + 1);
        if ((int64_t)*dest == (int64_t)*s0 && (int64_t)*t0 == (int64_t)*s1) {
          *s2 = *src2;
          *t0 = rs2 == 0 ? 0 : *(src2 + 1);
          rtl_sm(s, s2, src1, 0, 8, MMU_DYNAMIC);
          rtl_sm(s, t0, src1, 8, 8, MMU_DYNAMIC);
        }
        if (rd) {
          rtl_mv(s, dest, s0);
          rtl_mv(s, dest + 1, s1);
        }
        break;
      default : assert(0);
    }
    cpu.amo = false;
    return ;
  }
#endif // CONFIG_RV_ZACAS

  cpu.amo = true;
  rtl_lms(s, s0, src1, 0, width, MMU_DYNAMIC);
  switch (funct5) {
    case 0b00001: rtl_mv (s, s1, src2); break;
    case 0b00000: rtl_add(s, s1, s0, src2); break;
    case 0b01000: rtl_or (s, s1, s0, src2); break;
    case 0b01100: rtl_and(s, s1, s0, src2); break;
    case 0b00100: rtl_xor(s, s1, s0, src2); break;
    case 0b10000: // amomin
      if (width == 8) *s1 = ((int64_t)*s0 < (int64_t)*src2 ? *s0 : *src2);
      else *s1 = ((int32_t)*s0 < (int32_t)*src2 ? *s0 : *src2);
      break;
    case 0b10100: // amomax
      if (width == 8) *s1 = ((int64_t)*s0 > (int64_t)*src2 ? *s0 : *src2);
      else *s1 = ((int32_t)*s0 > (int32_t)*src2 ? *s0 : *src2);
      break;
    case 0b11000: // amominu
      if (width == 8) *s1 = ((uint64_t)*s0 < (uint64_t)*src2 ? *s0 : *src2);
      else *s1 = ((uint32_t)*s0 < (uint32_t)*src2 ? *s0 : *src2);
      break;
    case 0b11100: // amomaxu
      if (width == 8) *s1 = ((uint64_t)*s0 > (uint64_t)*src2 ? *s0 : *src2);
      else *s1 = ((uint32_t)*s0 > (uint32_t)*src2 ? *s0 : *src2);
      break;
    default: assert(0);
  }
  rtl_sm(s, s1, src1, 0, width, MMU_DYNAMIC);
  rtl_mv(s, dest, s0);
  cpu.amo = false;
}
