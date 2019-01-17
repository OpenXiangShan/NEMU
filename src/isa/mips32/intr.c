#include "cpu/rtl.h"
//#include "isa/mmu.h"

#define EX_ENTRY 0x180

void raise_intr(uint8_t NO, vaddr_t epc) {
  /* TODO: Trigger an interrupt/exception with ``NO''.
   * That is, use ``NO'' to index the IDT.
   */

  //TODO();
  cpu.cause = NO << 2;
  cpu.epc = epc;

  rtl_j(EX_ENTRY);
}
