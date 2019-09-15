#include "rtl/rtl.h"
#include "csr.h"

#define INTR_BIT (1ULL << 63)

void raise_intr(word_t NO, vaddr_t epc) {
  /* TODO: Trigger an interrupt/exception with ``NO''.
   * That is, use ``NO'' to index the IDT.
   */

  mcause->val = NO;
  mepc->val = epc;
  mstatus->mpie = mstatus->mie;
  mstatus->mie = 0;
  rtl_li(&s0, mtvec->val);
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
