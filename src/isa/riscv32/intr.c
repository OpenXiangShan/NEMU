#include <cpu/exec.h>
#include "local-include/rtl.h"

void raise_intr(DecodeExecState *s, uint32_t NO, vaddr_t epc) {
  /* TODO: Trigger an interrupt/exception with ``NO''.
   * That is, use ``NO'' to index the IDT.
   */

  cpu.scause = NO;
  cpu.sepc = epc;
  cpu.sstatus.spie = cpu.sstatus.sie;
  cpu.sstatus.sie = 0;
  rtl_li(s, s0, cpu.stvec);
  rtl_jr(s, s0);
}

void query_intr(DecodeExecState *s) {
  if (cpu.INTR && cpu.sstatus.sie) {
    cpu.INTR = false;
    raise_intr(s, 0x80000005, cpu.pc);
    update_pc(s);
  }
}
