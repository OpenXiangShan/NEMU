#include "rtl/rtl.h"
#include "csr.h"

#define INTR_BIT (1ULL << 63)

void raise_intr(word_t NO, vaddr_t epc) {
  // TODO: Trigger an interrupt/exception with ``NO''

  word_t deleg = (NO & INTR_BIT ? mideleg->val : medeleg->val);
  bool delegS = ((deleg & (1 << (NO & 0xf))) != 0) && (cpu.mode < MODE_M);

  if (delegS) {
    scause->val = NO;
    sepc->val = epc;
    mstatus->spp = cpu.mode;
    mstatus->spie = mstatus->sie;
    mstatus->sie = 0;
    cpu.mode = MODE_S;
    rtl_li(&s0, stvec->val);
  } else {
    mcause->val = NO;
    mepc->val = epc;
    mstatus->mpp = cpu.mode;
    mstatus->mpie = mstatus->mie;
    mstatus->mie = 0;
    cpu.mode = MODE_M;
    rtl_li(&s0, mtvec->val);
  }

  rtl_jr(&s0);
}

bool isa_query_intr(void) {
  extern bool clint_query_intr(void);
  if (mstatus->mie) {
    //if (cpu.INTR) {
    //  cpu.INTR = false;
    //  // machine external interrupt
    //  raise_intr(0xb | INTR_BIT, cpu.pc);
    //}
    if (clint_query_intr() && mie->mtie) {
      // machine timer interrupt
      raise_intr(0x7 | INTR_BIT, cpu.pc);
      return true;
    }
  }
  return false;
}
