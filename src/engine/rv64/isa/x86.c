#ifdef __ISA_x86__

#include <cpu/decode.h>
#include <rtl/rtl.h>
#include <isa.h>
#include <isa/riscv64.h>
#include "../tran.h"

uint32_t rtlreg2varidx(DecodeExecState *s, const rtlreg_t* dest) {
  rtlreg_t* gpr_start = (rtlreg_t *)cpu.gpr;
  rtlreg_t* gpr_end = (void *)gpr_start + sizeof(cpu.gpr);

  if (dest >= gpr_start && dest < gpr_end)
    return (dest - gpr_start)|0x10;//from x16 to x23

#define CASE(ptr, idx) if (dest == ptr) return idx;
  CASE(rz, 0)
  CASE(s0, 2)
  CASE(s1, 3)
  CASE(s2, 4)
  CASE(t0, 5)
  CASE(&id_src1->val, 7)
  CASE(&id_src2->val, 8)
  CASE(&id_dest->val, 9)
  CASE(&s->isa.mbr, 10)
#ifdef LAZY_CC
  CASE(&cpu.cc_dest, 13)
  CASE(&cpu.cc_src1, 14)
  CASE(&cpu.cc_src2, 15)
#else
  CASE(&cpu.CF, 13)
  CASE(&cpu.OF, 14)
  CASE(&cpu.ZF, 15)
  CASE(&cpu.SF, 1)
#endif
  panic("bad ptr = %p", dest);
}

void guest_init() {
}

void guest_getregs(CPU_state *x86) {
  riscv64_CPU_state r;
  backend_getregs(&r);
  int i;
  for (i = 0; i < 8; i ++) {
    x86->gpr[i]._32 = r.gpr[i + 0x10]._64;
  }
}

void guest_setregs(const CPU_state *x86) {
  riscv64_CPU_state r;
  backend_getregs(&r);
  int i;
  for (i = 0; i < 8; i ++) {
    r.gpr[i + 0x10]._64 = x86->gpr[i]._32;
  }
  backend_setregs(&r);
}

#endif
