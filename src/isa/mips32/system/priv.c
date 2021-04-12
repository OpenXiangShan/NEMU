#include "../local-include/rtl.h"
#include "../local-include/intr.h"
#include <cpu/difftest.h>

void tlbwr();
void tlbwi();
void tlbp();

static void csrrw(rtlreg_t *dest, const rtlreg_t *src, uint32_t csrid) {
  if (dest != NULL) {
    switch (csrid) {
      case 0:  *dest = cpu.index; break;
      case 10: *dest = cpu.entryhi.val; break;
      case 12: *dest = cpu.status.val; break;
      case 13: *dest = cpu.cause;
               // qemu may set cause.IP[7]
               IFNDEF(CONFIG_DIFFTEST_REF_NEMU, difftest_skip_ref());
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
