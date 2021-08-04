#include "../local-include/rtl.h"
#include "../local-include/intr.h"

#ifndef __ICS_EXPORT
word_t raise_intr(uint32_t NO, vaddr_t epc) {
  cpu.scause = NO;
  cpu.sepc = epc;
  cpu.sstatus.spie = cpu.sstatus.sie;
  cpu.sstatus.sie = 0;
  return cpu.stvec;
}

word_t isa_query_intr() {
  if (cpu.INTR && cpu.sstatus.sie) {
    cpu.INTR = false;
    return 0x80000005;
  }
  return INTR_EMPTY;
}

#else
word_t raise_intr(uint32_t NO, vaddr_t epc) {
  /* TODO: Trigger an interrupt/exception with ``NO''.
   * That is, use ``NO'' to index the IDT.
   */

  return 0;
}

word_t isa_query_intr() {
  return INTR_EMPTY;
}
#endif
