#include <isa.h>

#ifndef __ICS_EXPORT
word_t raise_intr(word_t NO, vaddr_t epc) {
  cpu.mcause = NO;
  cpu.mepc = epc;
  cpu.mstatus.mpp = cpu.mode;
  cpu.mode = 3;
  cpu.mstatus.mpie = cpu.mstatus.mie;
  cpu.mstatus.mie = 0;
  return cpu.mtvec;
}

word_t isa_query_intr() {
  if (cpu.INTR && cpu.mstatus.mie) {
    cpu.INTR = false;
    return 0x80000007;
  }
  return INTR_EMPTY;
}

#else
word_t raise_intr(word_t NO, vaddr_t epc) {
  /* TODO: Trigger an interrupt/exception with ``NO''.
   * That is, use ``NO'' to index the IDT.
   */

  return 0;
}

word_t isa_query_intr() {
  return INTR_EMPTY;
}
#endif
