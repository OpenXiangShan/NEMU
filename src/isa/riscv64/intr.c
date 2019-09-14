#include "rtl/rtl.h"
#include "csr.h"

void raise_intr(uint32_t NO, vaddr_t epc) {
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
  if (cpu.INTR && mstatus->mie) {
    cpu.INTR = false;
    // machine external interrupt
    raise_intr(0x8000000b, cpu.pc);
    return true;
  }
  return false;
}
