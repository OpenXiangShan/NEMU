#include "../local-include/csr.h"
#include "../local-include/intr.h"
#include <rtl/fp.h>
#include <cpu/cpu.h>

bool fp_enable() {
  return (mstatus->fs != 0);
}

void fp_set_dirty() {
  mstatus->sd = 1;
  mstatus->fs = 3;
}

uint32_t isa_fp_get_rm(Decode *s) {
  uint32_t rm = s->isa.instr.fp.rm;
  if (rm == 7) { rm = fcsr->frm; }
  if (rm <= 4) {
    switch (rm) {
      case 0: return FPCALL_RM_RNE;
      case 1: return FPCALL_RM_RTZ;
      case 2: return FPCALL_RM_RDN;
      case 3: return FPCALL_RM_RUP;
      case 4: return FPCALL_RM_RMM;
      default: assert(0);
    }
  }
  else {
    save_globals(s);
    longjmp_exception(EX_II);
  }
}

void isa_fp_set_ex(uint32_t ex) {
  uint32_t f = 0;
  if (ex & FPCALL_EX_NX) f |= 0x01;
  if (ex & FPCALL_EX_UF) f |= 0x02;
  if (ex & FPCALL_EX_OF) f |= 0x04;
  if (ex & FPCALL_EX_DZ) f |= 0x08;
  if (ex & FPCALL_EX_NV) f |= 0x10;
  fcsr->fflags.val = f;
  fp_set_dirty();
}
