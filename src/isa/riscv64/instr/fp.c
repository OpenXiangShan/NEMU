#include "../local-include/csr.h"
#include "../local-include/intr.h"
#include <cpu/cpu.h>

bool fp_enable() {
  return (mstatus->fs != 0);
}

void fp_set_dirty() {
  mstatus->sd = 1;
  mstatus->fs = 3;
}

uint32_t isa_fp_get_rm(Decode *s) {
  int rm = s->isa.instr.fp.rm;
  if (rm == 7) { rm = fcsr->frm; }
  if (rm <= 4) { return rm; }
  else {
    save_globals(s);
    longjmp_exception(EX_II);
  }
}

void isa_fp_update_ex_flags(Decode *s, uint32_t ex_flags) {
  fcsr->fflags.val |= ex_flags;
  fp_set_dirty();
}
