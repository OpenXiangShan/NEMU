#include "rtl/rtl.h"

void raise_intr(uint64_t NO, vaddr_t epc) {
  /* TODO: Trigger an interrupt/exception with ``NO''.
   * That is, use ``NO'' to index the IDT.
   */

  cpu.scause = NO;
  cpu.sepc = epc;
  cpu.sstatus.spie = cpu.sstatus.sie;
  cpu.sstatus.sie = 0;
  rtl_li(&s0, cpu.stvec);
  rtl_jr(&s0);
}

bool isa_query_intr(void) {
  if (cpu.INTR && cpu.sstatus.sie) {
    cpu.INTR = false;
    raise_intr(0x80000005, cpu.pc);
    return true;
  }
  return false;
}
