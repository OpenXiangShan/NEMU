#ifdef __ISA_mips32__

#include <cpu/decode.h>
#include <rtl/rtl.h>
#include <isa.h>
#include <isa/riscv64.h>
#include "../tran.h"

uint32_t reg_ptr2idx(DecodeExecState *s, const rtlreg_t* dest) {
  rtlreg_t* gpr_start = (rtlreg_t *)cpu.gpr;
  rtlreg_t* gpr_end = (void *)gpr_start + sizeof(cpu.gpr);

  if (dest >= gpr_start && dest < gpr_end) {
    int idx = dest - gpr_start;
    switch (idx) { // idx
      case 28: assert(0); break; // gp
      case 1: case 26: case 27: break; // at, k0, k1
      case 25: break; // t9
      default: return idx;
    }
  }

#define CASE(ptr, idx) if (dest == ptr) return idx;
  CASE(rz, 0)
  //CASE(&cpu.lo, 26)
  //CASE(&cpu.hi, 27)
  CASE(s0, 26)
  //CASE(s1, 27)
  panic("bad ptr = %p", dest);
}

void guest_getregs(CPU_state *mips32) {
  riscv64_CPU_state r;
  backend_getregs(&r);
  int i;
  for (i = 0; i < 32; i ++) {
    switch (i) {
      case 28: case 1: case 26: case 27: case 25: continue;
    }
    mips32->gpr[i]._32 = r.gpr[i]._64;
  }
}

void guest_setregs(const CPU_state *mips32) {
  riscv64_CPU_state r;
  backend_getregs(&r);
  int i;
  for (i = 0; i < 32; i ++) {
    switch (i) {
      case 28: case 1: case 26: case 27: case 25: continue;
    }
    r.gpr[i]._64 = mips32->gpr[i]._32;
  }
  backend_setregs(&r);
}

#endif
