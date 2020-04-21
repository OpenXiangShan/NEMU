#ifdef __ISA_mips32__

#include <cpu/decode.h>
#include <rtl/rtl.h>
#include <isa.h>
#include <isa/riscv64.h>
#include "../tran.h"

uint32_t rtlreg2varidx(DecodeExecState *s, const rtlreg_t* dest) {
  rtlreg_t* gpr_start = (rtlreg_t *)cpu.gpr;
  rtlreg_t* gpr_end = (void *)gpr_start + sizeof(cpu.gpr);

  if (dest >= gpr_start && dest < gpr_end) {
    int rvidx = dest - gpr_start;
    switch (rvidx) {
      case tmp0:     return 1 | SPMIDX_MASK;   // fixed to tmp0
      case spm_base: return 2 | SPMIDX_MASK;   // used to store sratchpad addr
      case tmp_reg1: return 3 | SPMIDX_MASK;   // tmp_reg 1
      case tmp_reg2: return 4 | SPMIDX_MASK;   // tmp_reg 2
      case mask32:   return 5 | SPMIDX_MASK;   // fixed to mask32
      default: return rvidx;
    }
  }
  if (dest == rz) return 0;

  // other temps
  if (dest == &cpu.lo) return 6 | SPMIDX_MASK;
  if (dest == &cpu.hi) return 7 | SPMIDX_MASK;
  if (dest == s0)      return 8 | SPMIDX_MASK;
  if (dest == s1)      return 9 | SPMIDX_MASK;

  panic("bad ptr = %p", dest);
  return 0;
}

void guest_getregs(CPU_state *mips32) {
  riscv64_CPU_state r;
  backend_getregs(&r);
  int i;
  for (i = 0; i < 32; i ++) {
    switch (i) {
      case tmp0: case mask32: case spm_base: case tmp_reg1: case tmp_reg2: continue;
    }
    mips32->gpr[i]._32 = r.gpr[i]._64;
  }
  // FIXME: these are wrong
  mips32->lo = r.gpr[25]._64;
  mips32->hi = r.gpr[27]._64;
}

void guest_setregs(const CPU_state *mips32) {
  riscv64_CPU_state r;
  backend_getregs(&r);
  int i;
  for (i = 0; i < 32; i ++) {
    switch (i) {
      case tmp0: case mask32: case spm_base: case tmp_reg1: case tmp_reg2: continue;
    }
    r.gpr[i]._64 = mips32->gpr[i]._32;
  }
  // FIXME: these are wrong
  r.gpr[25]._64 = mips32->lo;
  r.gpr[27]._64 = mips32->hi;
  backend_setregs(&r);
}

#endif
