#ifdef __ISA_riscv64__

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
      case tmp_reg1: return 3 | SPMIDX_MASK;   // tmp_reg 1
      case tmp_reg2: return 4 | SPMIDX_MASK;   // tmp_reg 2
      default: return rvidx;
    }
  }
  if (dest == rz) return 0;

  // other temps
  if (dest == s0)      return 6 | SPMIDX_MASK;
  if (dest == s1)      return 7 | SPMIDX_MASK;

  panic("bad ptr = %p", dest);
  return 0;
}

int rtlreg_is_zero(DecodeExecState *s, const rtlreg_t* r) {
  return (r == rz) || (r == &cpu.gpr[0]._64);
}

void guest_getregs(CPU_state *riscv64) {
  riscv64_CPU_state r;
  backend_getregs(&r);
  int i;
  for (i = 0; i < 32; i ++) {
    switch (i) {
      case tmp0: case tmp_reg1: case tmp_reg2: continue;
    }
    riscv64->gpr[i]._64 = r.gpr[i]._64;
  }
}

void guest_setregs(const CPU_state *riscv64) {
  riscv64_CPU_state r;
  backend_getregs(&r);
  int i;
  for (i = 0; i < 32; i ++) {
    switch (i) {
      case tmp0: case tmp_reg1: case tmp_reg2: continue;
    }
    r.gpr[i]._64 = riscv64->gpr[i]._64;
  }
  backend_setregs(&r);
}

#endif
