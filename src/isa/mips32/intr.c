#include <cpu/exec.h>
#include <monitor/difftest.h>
#include "local-include/rtl.h"
#include "local-include/intr.h"
#include <setjmp.h>

#define EX_ENTRY 0x80000180

void raise_intr(DecodeExecState *s, uint32_t NO, vaddr_t epc) {
  /* TODO: Trigger an interrupt/exception with ``NO''.
   * That is, use ``NO'' to index the IDT.
   */

  //TODO();
  vaddr_t target = (NO & TLB_REFILL) ? 0x80000000 : EX_ENTRY;
  NO &= ~TLB_REFILL;
  cpu.cause = NO << 2;
  cpu.epc = epc;
  cpu.status.exl = 1;

  rtl_j(s, target);

  difftest_skip_dut(1, 2);
}

void query_intr(DecodeExecState *s) {
  if (cpu.INTR && (cpu.status.ie) && !(cpu.status.exl)) {
    cpu.INTR = false;
    raise_intr(s, 0, cpu.pc);
    update_pc(s);
  }
}
