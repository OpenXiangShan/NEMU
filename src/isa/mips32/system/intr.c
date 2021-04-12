#include "../local-include/rtl.h"
#include "../local-include/intr.h"

#ifndef __ICS_EXPORT
#include <cpu/difftest.h>

word_t raise_intr(uint32_t NO, vaddr_t epc) {
#define EX_ENTRY 0x80000180
  vaddr_t target = (NO & TLB_REFILL) ? 0x80000000 : EX_ENTRY;
  NO &= ~TLB_REFILL;
  cpu.cause = NO << 2;
  cpu.epc = epc;
  cpu.status.exl = 1;

  difftest_skip_dut(1, 2);

  return target;
}

void isa_query_intr() {
  if (cpu.INTR && (cpu.status.ie) && !(cpu.status.exl)) {
    cpu.INTR = false;
    cpu.pc = raise_intr(0, cpu.pc);
  }
}
#else
word_t raise_intr(uint32_t NO, vaddr_t epc) {
  /* TODO: Trigger an interrupt/exception with ``NO''.
   * That is, use ``NO'' to index the IDT.
   */

  return 0;
}

void isa_query_intr() {
}
#endif
