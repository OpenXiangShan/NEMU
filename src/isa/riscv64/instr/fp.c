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

static uint32_t nemu_rm_cache = 0;
void fp_update_rm_cache(uint32_t rm) {
  switch (rm) {
    case 0: nemu_rm_cache = FPCALL_RM_RNE; return;
    case 1: nemu_rm_cache = FPCALL_RM_RTZ; return;
    case 2: nemu_rm_cache = FPCALL_RM_RDN; return;
    case 3: nemu_rm_cache = FPCALL_RM_RUP; return;
    case 4: nemu_rm_cache = FPCALL_RM_RMM; return;
    default: assert(0);
  }
}

bool fp_enable() {
  return MUXDEF(CONFIG_MODE_USER, true, mstatus->fs != 0);
}

void fp_set_dirty() {
  // lazily update mstatus->sd when reading mstatus
#if defined (CONFIG_SHARE) || defined (CONFIG_DIFFTEST_REF_SPIKE)
  mstatus->sd = 1;
#endif
// Spike update fs and sd in the meantime
  mstatus->fs = 3;
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
    default: save_globals(s); longjmp_exception(EX_II);
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
#endif // CONFIG_FPU_NONE
}

uint32_t isa_fp_get_frm() {
#ifndef CONFIG_FPU_NONE
  return fcsr->frm;
#endif // CONFIG_FPU_NONE
  return 0;
}
