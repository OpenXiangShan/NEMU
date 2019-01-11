#include "cpu/rtl.h"
#include "memory/mmu.h"

void raise_intr(uint8_t NO, vaddr_t ret_addr) {
  /* TODO: Trigger an interrupt/exception with ``NO''.
   * That is, use ``NO'' to index the IDT.
   */

  //TODO();
  vaddr_t base = cpu.idtr.base + (NO << 3);
  rtl_li(&t3, base);
  rtl_lm(&t0, &t3, 4);
  rtl_addi(&t3, &t3, 4);
  rtl_lm(&t1, &t3, 4);

  // check the present bit
  assert((t1 >> 15) & 0x1);

  void rtl_compute_eflags(rtlreg_t *dest);
  rtl_compute_eflags(&t2);
  rtl_push(&t2);
  rtl_li(&t2, cpu.cs);
  rtl_push(&t2);   // cs
  rtl_li(&t2, ret_addr);
  rtl_push(&t2);

  rtl_li(&cpu.IF, 0);

  rtl_andi(&t0, &t0, 0xffff);
  rtl_andi(&t1, &t1, 0xffff0000);
  rtl_or(&t0, &t0, &t1);
  rtl_jr(&t0);
}
