#ifdef __ISA_mips32__

#include <cpu/decode.h>
#include <rtl/rtl.h>
#include <isa.h>
#include <isa/riscv64.h>
#include "../tran.h"
#include "../spill.h"

uint32_t reg_ptr2tmpidx(DecodeExecState *s, const rtlreg_t* dest) {
  rtlreg_t* gpr_start = (rtlreg_t *)cpu.gpr;
  rtlreg_t* gpr_end = (void *)gpr_start + sizeof(cpu.gpr);

  if (dest >= gpr_start && dest < gpr_end) {
    int idx = dest - gpr_start;
    switch (idx) {
      case 1:            return 1; break;   // fixed to tmp0
      case spm_base:     return 2; break;   // used to store sratchpad addr
      case TMP_REG_1:    return 3; break;   // tmp_reg 1
      case TMP_REG_2:    return 4; break;   // tmp_reg 2
      case 28:           return 5; break;   // fixed to mask32
      default: return 0;
    }
  }
  if (dest == rz) return 0;

  // other temps
  if (dest == &cpu.lo) return 6;
  if (dest == &cpu.hi) return 7;
  if (dest == s0)      return 8;
  if (dest == s1)      return 9;

  panic("bad ptr = %p", dest);
  return 0;
}

uint32_t reg_ptr2idx(DecodeExecState *s, const rtlreg_t* dest) {
  rtlreg_t* gpr_start = (rtlreg_t *)cpu.gpr;
  rtlreg_t* gpr_end = (void *)gpr_start + sizeof(cpu.gpr);
  uint32_t tmp_idx = 0;  // index for sratchpad

  if (dest >= gpr_start && dest < gpr_end) {
    int idx = dest - gpr_start;
    switch (idx) {
      case 1:            tmp_idx = 1; break;   // fixed to tmp0
      case spm_base :    tmp_idx = 2; break;   // used to store sratchpad addr
      case TMP_REG_1:    tmp_idx = 3; break;   // tmp_reg 1
      case TMP_REG_2:    tmp_idx = 4; break;   // tmp_reg 2
      case 28:           tmp_idx = 5; break;   // fixed to mask32
      default: return idx;
    }
  }
  if (dest == rz) return 0;

  // other temps
  if (dest == &cpu.lo) tmp_idx = 6;
  if (dest == &cpu.hi) tmp_idx = 7;
  if (dest == s0)      tmp_idx = 8;
  if (dest == s1)      tmp_idx = 9;
  
  // unknown dest
  if (tmp_idx == 0) panic("bad ptr = %p", dest);

  // if tmp_reg has already mapped to dest, just return tmp_reg
  uint32_t idx;
  idx = check_tmp_reg(tmp_idx);
  if (idx) return idx;

  // if not mapped, spill out one tmp_reg and remap
  idx = spill_out_and_remap(s, tmp_idx);
  return idx;
}

void guest_getregs(CPU_state *mips32) {
  riscv64_CPU_state r;
  backend_getregs(&r);
  int i;
  for (i = 0; i < 32; i ++) {
    switch (i) {
      case 28: case 1: case spm_base: case TMP_REG_1: case TMP_REG_2: continue;
    }
    mips32->gpr[i]._32 = r.gpr[i]._64;
  }
  mips32->lo = r.gpr[25]._64;
  mips32->hi = r.gpr[27]._64;
}

void guest_setregs(const CPU_state *mips32) {
  riscv64_CPU_state r;
  backend_getregs(&r);
  int i;
  for (i = 0; i < 32; i ++) {
    switch (i) {
      case 28: case 1: case spm_base: case TMP_REG_1: case TMP_REG_2: continue;
    }
    r.gpr[i]._64 = mips32->gpr[i]._32;
  }
  r.gpr[25]._64 = mips32->lo;
  r.gpr[27]._64 = mips32->hi;
  backend_setregs(&r);
}

#endif
