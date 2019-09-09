#include "rtl/rtl.h"

void raise_intr(uint32_t NO, vaddr_t epc) {
  /* TODO: Trigger an interrupt/exception with ``NO''.
   * That is, use ``NO'' to index the IDT.
   */

  cpu.mcause = NO;
  cpu.mepc = epc;
  cpu.mstatus.mpie = cpu.mstatus.mie;
  cpu.mstatus.mie = 0;
  rtl_li(&s0, cpu.mtvec);
  rtl_jr(&s0);
}

bool isa_query_intr(void) {
  if (cpu.INTR && cpu.mstatus.mie) {
    cpu.INTR = false;
    raise_intr(0x80000005, cpu.pc);
    return true;
  }
  return false;
}
