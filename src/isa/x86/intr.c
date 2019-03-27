#include "rtl/rtl.h"
#include "isa/mmu.h"

void raise_intr(uint32_t NO, vaddr_t ret_addr) {
  /* TODO: Trigger an interrupt/exception with ``NO''.
   * That is, use ``NO'' to index the IDT.
   */

  //TODO();
  assert(NO < 256);
  vaddr_t base = cpu.idtr.base + (NO << 3);
  rtl_li(&s0, base);
  rtl_lm(&s1, &s0, 4);
  rtl_addi(&s0, &s0, 4);
  rtl_lm(&s0, &s0, 4);

  // check the present bit
  assert((s0 >> 15) & 0x1);

  // calcualte target address
  rtl_andi(&s1, &s1, 0xffff);
  rtl_andi(&s0, &s0, 0xffff0000);
  rtl_or(&s1, &s1, &s0);

  void rtl_compute_eflags(rtlreg_t *dest);
  rtl_compute_eflags(&s0);
  rtl_push(&s0);
  rtl_li(&s0, cpu.cs);
  rtl_push(&s0);   // cs
  rtl_li(&s0, ret_addr);
  rtl_push(&s0);

  rtl_li(&cpu.IF, 0);

  rtl_jr(&s1);
}

#define IRQ_TIMER 32
bool isa_query_intr(void) {
  if (cpu.INTR && cpu.IF) {
    cpu.INTR = false;
    raise_intr(IRQ_TIMER, cpu.pc);
    return true;
  }
  return false;
}
