#include "monitor/diff-test.h"
#include "rtl/rtl.h"
#include "isa/intr.h"
#include <setjmp.h>

#define EX_ENTRY 0x80000180

void raise_intr(uint32_t NO, vaddr_t epc) {
  /* TODO: Trigger an interrupt/exception with ``NO''.
   * That is, use ``NO'' to index the IDT.
   */

  //TODO();
  vaddr_t target = (NO & TLB_REFILL) ? 0x80000000 : EX_ENTRY;
  NO &= ~TLB_REFILL;
  cpu.cause = NO << 2;
  cpu.epc = epc;
  cpu.status.exl = 1;

  rtl_j(target);

  difftest_skip_dut(1, 2);
}

bool isa_query_intr(void) {
  if (cpu.INTR && (cpu.status.ie) && !(cpu.status.exl)) {
    cpu.INTR = false;
    raise_intr(0, cpu.pc);
    return true;
  }
  return false;
}

jmp_buf intr_buf;

void longjmp_raise_intr(uint32_t NO) {
  longjmp(intr_buf, NO + 1);
}
