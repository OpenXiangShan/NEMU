#include "../local-include/rtl.h"
#include "../local-include/intr.h"
#include <cpu/cpu.h>

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
  word_t tmp = (src != NULL ? *src : 0);
  if (dest != NULL) { *dest = *csr; }
  if (src != NULL) { *csr = tmp; }
}

static word_t priv_instr(uint32_t op, const rtlreg_t *src) {
  switch (op) {
    case 0x102: // sret
      cpu.sstatus.sie = cpu.sstatus.spie;
      cpu.sstatus.spie = 1;
      return cpu.sepc;
    case 0x120:; // sfence.vma
      mmu_tlb_flush(*src);
      return 0;
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
