#include "local-include/rtl.h"
#include "local-include/intr.h"

#ifndef __ICS_EXPORT
#include <monitor/difftest.h>

void tlbwr();
void tlbwi();
void tlbp();

word_t raise_intr(uint32_t NO, vaddr_t epc) {
#define EX_ENTRY 0x80000180
  vaddr_t target = (NO & TLB_REFILL) ? 0x80000000 : EX_ENTRY;
  NO &= ~TLB_REFILL;
  cpu.cause = NO << 2;
  cpu.epc = epc;
  cpu.status.exl = 1;

  difftest_skip_dut(1, 2);

  return target;
}

static void csrrw(rtlreg_t *dest, const rtlreg_t *src, uint32_t csrid) {
  if (dest != NULL) {
    switch (csrid) {
      case 0:  *dest = cpu.index; break;
      case 10: *dest = cpu.entryhi.val; break;
      case 12: *dest = cpu.status.val; break;
      case 13: *dest = cpu.cause;
#ifndef __DIFF_REF_NEMU__
               difftest_skip_ref(); // qemu may set cause.IP[7]
#endif
               break;
      case 14: *dest = cpu.epc; break;
      default: panic("Reading from CSR = %d is not supported", csrid);
    }
  }
  if (src != NULL) {
    switch (csrid) {
      case 0:  cpu.index       = *src; break;
      case 2:  cpu.entrylo0    = *src; break;
      case 3:  cpu.entrylo1    = *src; break;
      case 10: cpu.entryhi.val = *src & ~0x1f00; break;
      case 12: cpu.status.val  = *src; break;
      case 13: cpu.cause       = *src; break;
      case 14: cpu.epc         = *src; break;
      default: panic("Writing to CSR = %d is not supported", csrid);
    }
  }
}

static word_t priv_instr(uint32_t op, const rtlreg_t *src) {
  switch (op) {
    case PRIV_ERET:
      cpu.status.exl = 0;
      return cpu.epc;
    case PRIV_TLBWR: tlbwr(); break;
    case PRIV_TLBWI: tlbwi(); break;
    case PRIV_TLBP:  tlbp(); break;
    default: panic("Unsupported privilige operation = %d", op);
  }
  return 0;
}

void isa_hostcall(uint32_t id, rtlreg_t *dest, const rtlreg_t *src, uint32_t imm) {
  word_t ret = 0;
  switch (id) {
    case HOSTCALL_CSR: csrrw(dest, src, imm); return;
    case HOSTCALL_TRAP: ret = raise_intr(imm, *src); break;
    case HOSTCALL_PRIV: ret = priv_instr(imm, src); break;
    default: panic("Unsupported hostcall ID = %d", id);
  }
  if (dest) *dest = ret;
}

void query_intr() {
  if (cpu.INTR && (cpu.status.ie) && !(cpu.status.exl)) {
    cpu.INTR = false;
    cpu.pc = raise_intr(0, cpu.pc);
  }
}
#else
word_t raise_intr(uint32_t NO, vaddr_t epc) {
  /* TODO: Trigger an interrupt/exception with ``NO''.
   * That is, use ``NO'' to index the IDT.
   */

  return 0;
}

void query_intr() {
}
#endif
