#include "cpu/exec.h"

static inline rtlreg_t* csr_decode(uint32_t csr) {
  switch (csr) {
    case 0x180: return &cpu.satp.val;
    case 0x100: return &cpu.sstatus.val;
    case 0x105: return &cpu.stvec;
    case 0x141: return &cpu.sepc;
    case 0x142: return &cpu.scause;
    default: panic("unimplemented CSR 0x%x", csr);
  }
  return NULL;
}

make_EHelper(csrrw) {
  rtlreg_t *csr = csr_decode(id_src2->val);

  rtl_sr(id_dest->reg, csr, 4);
  rtl_mv(csr, &id_src->val);

  print_asm_template3("csrrw");

#if defined(DIFF_TEST)
  difftest_skip_dut();
#endif
}

make_EHelper(csrrs) {
  rtlreg_t *csr = csr_decode(id_src2->val);

  rtl_sr(id_dest->reg, csr, 4);
  rtl_or(csr, csr, &id_src->val);

  print_asm_template3("csrrs");

#if defined(DIFF_TEST)
  difftest_skip_dut();
#endif
}

extern void raise_intr(uint32_t NO, vaddr_t epc);

make_EHelper(priv) {
  uint32_t type = decinfo.isa.instr.csr;
  switch (type) {
    case 0:
      raise_intr(9, cpu.pc);
      print_asm("ecall");
      break;
    case 0x102:
      cpu.sstatus.sie = cpu.sstatus.spie;
      cpu.sstatus.spie = 1;
      rtl_li(&s0, cpu.sepc);
      rtl_jr(&s0);
      print_asm("sret");
      break;
    default: panic("unimplemented priv instruction type = 0x%x", type);
  }

#if defined(DIFF_TEST)
  difftest_skip_dut();
#endif
}
