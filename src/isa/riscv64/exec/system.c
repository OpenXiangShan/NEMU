#include "cpu/exec.h"
#include "../csr.h"

static inline word_t* csr_decode_wrapper(uint32_t addr) {
  difftest_skip_dut(1, 3);
  return csr_decode(addr);
}

make_EHelper(csrrw) {
  rtlreg_t *csr = csr_decode_wrapper(id_src2->val);

  rtl_sr(id_dest->reg, csr, 8);
  rtl_mv(csr, &id_src->val);

  print_asm_template3("csrrw");
}

make_EHelper(csrrs) {
  rtlreg_t *csr = csr_decode_wrapper(id_src2->val);

  rtl_sr(id_dest->reg, csr, 8);
  rtl_or(csr, csr, &id_src->val);

  print_asm_template3("csrrs");
}

make_EHelper(csrrc) {
  rtlreg_t *csr = csr_decode_wrapper(id_src2->val);

  rtl_sr(id_dest->reg, csr, 8);
  rtl_not(&s0, &id_src->val);
  rtl_and(csr, csr, &s0);

  print_asm_template3("csrrc");
}

extern void raise_intr(word_t NO, vaddr_t epc);

make_EHelper(priv) {
  uint32_t type = decinfo.isa.instr.csr;
  switch (type) {
    case 0:
      raise_intr(8 + cpu.mode, cpu.pc);
      print_asm("ecall");
      break;
    case 0x102:
      mstatus->sie = mstatus->spie;
      mstatus->spie = 1;
      change_mode(mstatus->spp);
      mstatus->mpp = MODE_U;
      rtl_li(&s0, sepc->val);
      rtl_jr(&s0);
      print_asm("sret");
      break;
    case 0x120:
      print_asm("sfence.vma");
      break;
    case 0x302:
      mstatus->mie = mstatus->mpie;
      mstatus->mpie = 1;
      change_mode(mstatus->mpp);
      mstatus->mpp = MODE_U;
      rtl_li(&s0, mepc->val);
      rtl_jr(&s0);
      print_asm("mret");
      break;
    default: panic("unimplemented priv instruction type = 0x%x", type);
  }

  difftest_skip_dut(1, 2);
}
