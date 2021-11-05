#include "../local-include/csr.h"
#include <rtl/fp.h>

static uint32_t nemu_rm_cache = 0;
void fp_update_rm_cache(uint32_t rm) { nemu_rm_cache = rm; }

bool fp_enable() {
  IFDEF(CONFIG_FPU_NONE, return false);
  IFDEF(CONFIG_MODE_USER, return true);
  return mstatus->fs != 0;
}

void fp_set_dirty() {
  // lazily update mstatus->sd when reading mstatus
  mstatus->fs = 3;
}

uint32_t isa_fp_translate_rm(uint32_t riscv64_rm) {
  if (riscv64_rm == 0b111) riscv64_rm = nemu_rm_cache;
  uint32_t table[] = {
    FPCALL_RM_RNE,
    FPCALL_RM_RTZ,
    FPCALL_RM_RDN,
    FPCALL_RM_RUP,
    FPCALL_RM_RMM
  };
  return table[riscv64_rm];
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
