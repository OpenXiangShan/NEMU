#include "cpu/rtl.h"
#include "isa/intr.h"
#include <setjmp.h>

#define EX_ENTRY 0x80000180

void raise_intr(uint8_t NO, vaddr_t epc) {
  /* TODO: Trigger an interrupt/exception with ``NO''.
   * That is, use ``NO'' to index the IDT.
   */

  //TODO();
  vaddr_t target = (NO & TLB_REFILL) ? 0x80000000 : EX_ENTRY;
  NO &= ~TLB_REFILL;
  cpu.cause = NO << 2;
  cpu.epc = epc;

  rtl_j(target);
}

bool isa_query_intr(void) {
  if (cpu.INTR && (cpu.status & 0x1)) {
    cpu.INTR = false;
    raise_intr(0, cpu.pc);
    return true;
  }
  return false;
}

jmp_buf intr_buf;

void longjmp_raise_intr(uint8_t NO) {
  longjmp(intr_buf, NO + 1);
}
