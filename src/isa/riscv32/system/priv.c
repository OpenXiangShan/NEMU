#include <rtl/rtl.h>
#ifndef __ICS_EXPORT
#include <cpu/cpu.h>

static word_t* csr_decode(uint32_t csr) {
  switch (csr) {
    case 0x180: return &cpu.satp.val;
    case 0x300: return &cpu.mstatus.val;
    case 0x305: return &cpu.mtvec;
    case 0x340: return &cpu.mscratch;
    case 0x341: return &cpu.mepc;
    case 0x342: return &cpu.mcause;
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
    case 0x302: // mret
      cpu.mode = cpu.mstatus.mpp;
      cpu.mstatus.mpp = 0;
      cpu.mstatus.mie = cpu.mstatus.mpie;
      cpu.mstatus.mpie = 1;
      return cpu.mepc;
    case 0x120:; // sfence.vma
      mmu_tlb_flush(*src);
      return 0;
    default: panic("Unsupported privilige operation = %d", op);
  }
  return 0;
}

void isa_hostcall(uint32_t id, rtlreg_t *dest,
    const rtlreg_t *src1, const rtlreg_t *src2, word_t imm) {
  word_t ret = 0;
  switch (id) {
    case HOSTCALL_CSR: csrrw(dest, src1, imm); return;
    case HOSTCALL_TRAP: ret = raise_intr(imm, *src1); break;
    case HOSTCALL_PRIV: ret = priv_instr(imm, src1); break;
    default: panic("Unsupported hostcall ID = %d", id);
  }
  if (dest) *dest = ret;
}
#else
void isa_hostcall(uint32_t id, rtlreg_t *dest,
    const rtlreg_t *src1, const rtlreg_t *src2, word_t imm) {
  word_t ret = 0;
  switch (id) {
    default: panic("Unsupported hostcall ID = %d", id);
  }
  if (dest) *dest = ret;
}
#endif
