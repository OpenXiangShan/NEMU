#include <cpu/exec.h>
#include "monitor/difftest.h"
#include "local-include/rtl.h"

#ifndef __ICS_EXPORT
void raise_intr(DecodeExecState *s, uint32_t NO, vaddr_t ret_addr) {
  assert(NO < 256);
  int old_cs = cpu.sreg[SR_CS].val;
  if (NO < 32) {
    // internal exception does not check priviledge
    // fetch the gate descriptor with ring 0
    cpu.sreg[SR_CS].rpl = 0;
  }
  rtl_li(s, s0, cpu.idtr.base);
  rtl_lm(s, s1, s0, NO << 3, 4);
  rtl_lm(s, s0, s0, (NO << 3) + 4, 4);

  // check the present bit
  assert((*s0 >> 15) & 0x1);

  uint16_t new_cs = *s1 >> 16;

  // calcualte target address
  rtl_andi(s, s1, s1, 0xffff);
  rtl_andi(s, s0, s0, 0xffff0000);
  rtl_or(s, s1, s1, s0);

  if ((new_cs & 0x3) < (old_cs & 0x3)) {
    // stack switch
    assert(cpu.sreg[SR_TR].ti == 0); // check the table bit
    uint32_t tss_desc_base = cpu.gdtr.base + (cpu.sreg[SR_TR].idx << 3);
    uint32_t desc_lo = vaddr_read(tss_desc_base + 0, 4);
    uint32_t desc_hi = vaddr_read(tss_desc_base + 4, 4);
    assert((desc_hi >> 15) & 0x1); // check the present bit
    assert((desc_hi & 0x1d00) == 0x900); // check type
    assert((desc_hi & 0x200) == 0); // check the busy bit
    uint32_t tss_base = (desc_hi & 0xff000000) | ((desc_hi & 0xff) << 16) | (desc_lo >> 16);

    assert((old_cs & 0x3) == 3); // only support switching from ring 3
    assert((new_cs & 0x3) == 0); // only support switching to ring 0

    uint32_t esp3 = cpu.esp;
    uint32_t ss3  = cpu.sreg[SR_SS].val;
    cpu.esp = vaddr_read(tss_base + 4, 4);
    cpu.sreg[SR_SS].val = vaddr_read(tss_base + 8, 2);

    rtl_li(s, s0, ss3);
    rtl_push(s, s0);
    rtl_li(s, s0, esp3);
    rtl_push(s, s0);
  }

  void rtl_compute_eflags(DecodeExecState *s, rtlreg_t *dest);
  rtl_compute_eflags(s, s0);
  rtl_push(s, s0);
  rtl_li(s, s0, old_cs);
  rtl_push(s, s0);   // cs
  rtl_li(s, s0, ret_addr);
  rtl_push(s, s0);

  if (NO == 14) {
    // page fault has error code
    rtl_li(s, s0, cpu.error_code);
    rtl_push(s, s0);
  }

  rtl_mv(s, &cpu.IF, rz);
  cpu.sreg[SR_CS].val = new_cs;

  rtl_jr(s, s1);

#ifdef DIFF_TEST
  if (ref_difftest_raise_intr) ref_difftest_raise_intr(NO);
#endif
}

#define IRQ_TIMER 32

void query_intr(DecodeExecState *s) {
  if (cpu.INTR && cpu.IF) {
    cpu.INTR = false;
    raise_intr(s, IRQ_TIMER, cpu.pc);
    update_pc(s);
  }
}
#else
void raise_intr(DecodeExecState *s, uint32_t NO, vaddr_t ret_addr) {
  /* TODO: Trigger an interrupt/exception with ``NO''.
   * That is, use ``NO'' to index the IDT.
   */

  TODO();
}

void query_intr(DecodeExecState *s) {
  TODO();
}
#endif
