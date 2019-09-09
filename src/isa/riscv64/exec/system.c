#include "cpu/exec.h"

static inline rtlreg_t* csr_decode(uint32_t csr) {
  difftest_skip_dut(1, 3);

  switch (csr) {
    case 0x180: return &cpu.satp.val;
    case 0x300: return &cpu.mstatus.val;
    case 0x305: return &cpu.mtvec;
    case 0x341: return &cpu.mepc;
    case 0x342: return &cpu.mcause;
    default: panic("unimplemented CSR 0x%x", csr);
  }
  return NULL;
}

make_EHelper(csrrw) {
  rtlreg_t *csr = csr_decode(id_src2->val);

  rtl_sr(id_dest->reg, csr, 8);
  rtl_mv(csr, &id_src->val);

  print_asm_template3("csrrw");
}

make_EHelper(csrrs) {
  rtlreg_t *csr = csr_decode(id_src2->val);

  rtl_sr(id_dest->reg, csr, 8);
  rtl_or(csr, csr, &id_src->val);

  print_asm_template3("csrrs");
}

extern void raise_intr(uint32_t NO, vaddr_t epc);

make_EHelper(priv) {
  uint32_t type = decinfo.isa.instr.csr;
  switch (type) {
    case 0:
      raise_intr(11, cpu.pc);
      print_asm("ecall");
      break;
    case 0x302:
      cpu.mstatus.mie = cpu.mstatus.mpie;
      cpu.mstatus.mpie = 1;
      rtl_li(&s0, cpu.mepc);
      rtl_jr(&s0);
      print_asm("mret");
      break;
    default: panic("unimplemented priv instruction type = 0x%x", type);
  }

  difftest_skip_dut(1, 2);
}
