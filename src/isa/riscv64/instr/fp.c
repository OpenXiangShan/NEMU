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

#include "../local-include/csr.h"
#include "../local-include/intr.h"
#include <rtl/fp.h>
#include <cpu/cpu.h>
#include <cpu/difftest.h>

static uint32_t nemu_rm_cache = 0;
void fp_update_rm_cache(uint32_t rm) {
  switch (rm) {
    case 0: nemu_rm_cache = FPCALL_RM_RNE; return;
    case 1: nemu_rm_cache = FPCALL_RM_RTZ; return;
    case 2: nemu_rm_cache = FPCALL_RM_RDN; return;
    case 3: nemu_rm_cache = FPCALL_RM_RUP; return;
    case 4: nemu_rm_cache = FPCALL_RM_RMM; return;
    default: nemu_rm_cache = rm;
  }
}

bool fp_enable() {
#ifdef CONFIG_MODE_USER
  return true;
#else // !CONFIG_MODE_USER
  if (mstatus->fs == 0) {
    return false;
  }
#ifdef CONFIG_RVH
  if (cpu.v && vsstatus->fs == 0) {
    return false;
  }
#endif // CONFIG_RVH
  return true;
#endif // CONFIG_MODE_USER
}

void fp_set_dirty() {
  mstatus->fs = EXT_CONTEXT_DIRTY;
#ifdef CONFIG_RVH
  if (cpu.v == 1) {
    vsstatus->fs = EXT_CONTEXT_DIRTY;
  }
#endif //CONFIG_RVH
  IFDEF(CONFIG_DIFFTEST_DIRTY_FS_VS, ref_difftest_dirty_fsvs(SSTATUS_FS));
}

uint32_t isa_fp_get_rm(Decode *s) {
  uint32_t rm = s->isa.instr.fp.rm;
  if (likely(rm == 7)) { return nemu_rm_cache; }
  switch (rm) {
    case 0: return FPCALL_RM_RNE;
    case 1: return FPCALL_RM_RTZ;
    case 2: return FPCALL_RM_RDN;
    case 3: return FPCALL_RM_RUP;
    case 4: return FPCALL_RM_RMM;
    default: return rm;
  }
}

void isa_fp_set_ex(uint32_t ex) {
#ifndef CONFIG_FPU_NONE
  uint32_t f = 0;
  if (ex & FPCALL_EX_NX) f |= 0x01;
  if (ex & FPCALL_EX_UF) f |= 0x02;
  if (ex & FPCALL_EX_OF) f |= 0x04;
  if (ex & FPCALL_EX_DZ) f |= 0x08;
  if (ex & FPCALL_EX_NV) f |= 0x10;
  fcsr->fflags.val = fcsr->fflags.val | f;
  fflags->val = fcsr->fflags.val;
  fp_set_dirty();
#endif // CONFIG_FPU_NONE
}

void isa_fp_csr_check() {
#ifndef CONFIG_FPU_NONE
  if(unlikely(mstatus->fs == 0)){
    longjmp_exception(EX_II);
    assert(0);
  }
  #ifdef CONFIG_RVH
  if (cpu.v && vsstatus->fs == 0){
    longjmp_exception(EX_II);
  }
  #endif
#endif // CONFIG_FPU_NONE
}

void isa_fp_rm_check(uint32_t rm) {
#ifndef CONFIG_FPU_NONE
  if (rm > 4) {
    longjmp_exception(EX_II);
  }
#endif // CONFIG_FPU_NONE
}

uint32_t isa_fp_get_frm(void) {
#ifndef CONFIG_FPU_NONE
  return fcsr->frm;
#endif // CONFIG_FPU_NONE
  return 0;
}
