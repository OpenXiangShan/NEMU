#include "cpu/exec.h"
#include "rtl/rtl.h"

void raise_intr(DecodeExecState *s, uint32_t NO, vaddr_t ret_addr) {
  /* TODO: Trigger an interrupt/exception with ``NO''.
   * That is, use ``NO'' to index the IDT.
   */

  //TODO();
  assert(NO < 256);
  vaddr_t base = cpu.idtr.base + (NO << 3);
  rtl_li(s, s0, base);
  rtl_lm(s, s1, s0, 4);
  rtl_addi(s, s0, s0, 4);
  rtl_lm(s, s0, s0, 4);

  // check the present bit
  assert((*s0 >> 15) & 0x1);

  // calcualte target address
  rtl_andi(s, s1, s1, 0xffff);
  rtl_andi(s, s0, s0, 0xffff0000);
  rtl_or(s, s1, s1, s0);

  void rtl_compute_eflags(DecodeExecState *s, rtlreg_t *dest);
  rtl_compute_eflags(s, s0);
  rtl_push(s, s0);
  rtl_li(s, s0, cpu.cs);
  rtl_push(s, s0);   // cs
  rtl_li(s, s0, ret_addr);
  rtl_push(s, s0);

  rtl_li(s, &cpu.IF, 0);

  rtl_jr(s, s1);
}

#define IRQ_TIMER 32
void query_intr(DecodeExecState *s) {
  if (cpu.INTR && cpu.IF) {
    cpu.INTR = false;
    raise_intr(s, IRQ_TIMER, cpu.pc);
    update_pc(s);
  }
}
