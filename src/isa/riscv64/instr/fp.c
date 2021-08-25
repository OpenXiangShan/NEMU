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
#ifdef CONFIG_SHARE
  mstatus->sd = 1;
#endif
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
  uint32_t f = 0;
  if (ex & FPCALL_EX_NX) f |= 0x01;
  if (ex & FPCALL_EX_UF) f |= 0x02;
  if (ex & FPCALL_EX_OF) f |= 0x04;
  if (ex & FPCALL_EX_DZ) f |= 0x08;
  if (ex & FPCALL_EX_NV) f |= 0x10;
  fcsr->fflags.val = fcsr->fflags.val | f;
  fp_set_dirty();
}

void isa_fp_csr_check() {
  if(unlikely(mstatus->fs == 0)){
    longjmp_exception(EX_II);
    assert(0);
  }
}
