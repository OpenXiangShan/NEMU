#include "local-include/rtl.h"
#include "local-include/intr.h"

#ifndef __ICS_EXPORT
word_t raise_intr(uint32_t NO, vaddr_t epc) {
  cpu.scause = NO;
  cpu.sepc = epc;
  cpu.sstatus.spie = cpu.sstatus.sie;
  cpu.sstatus.sie = 0;
  return cpu.stvec;
}

static inline word_t* csr_decode(uint32_t csr) {
  switch (csr) {
    case 0x180: return &cpu.satp.val;
    case 0x100: return &cpu.sstatus.val;
    case 0x105: return &cpu.stvec;
    case 0x140: return &cpu.sscratch;
    case 0x141: return &cpu.sepc;
    case 0x142: return &cpu.scause;
    default: panic("unimplemented CSR 0x%x", csr);
  }
  return NULL;
}

static void csrrw(rtlreg_t *dest, const rtlreg_t *src, uint32_t csrid) {
  word_t *csr = csr_decode(csrid);
  word_t tmp = *csr;
  if (src != NULL) { *csr = *src; }
  if (dest != NULL) { *dest = tmp; }
}

static word_t priv_instr(uint32_t op, const rtlreg_t *src) {
  switch (op) {
    case 0x102: // sret
      cpu.sstatus.sie = cpu.sstatus.spie;
      cpu.sstatus.spie = 1;
      return cpu.sepc;
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
  if (cpu.INTR && cpu.sstatus.sie) {
    cpu.INTR = false;
    cpu.pc = raise_intr(0x80000005, cpu.pc);
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
