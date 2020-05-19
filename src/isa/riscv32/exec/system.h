static inline rtlreg_t* csr_decode(uint32_t csr) {
#ifndef __DIFF_REF_NEMU__
  difftest_skip_dut(1, 3);
#endif

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

static inline make_EHelper(csrrw) {
  rtlreg_t *csr = csr_decode(id_src2->imm);

  if (ddest == dsrc1) {
    rtl_mv(s, s0, csr);
    rtl_mv(s, csr, dsrc1);
    rtl_mv(s, dsrc1, s0);
  } else {
    rtl_mv(s, ddest, csr);
    rtl_mv(s, csr, dsrc1);
  }

  print_asm_template3("csrrw");
}

static inline make_EHelper(csrrs) {
  rtlreg_t *csr = csr_decode(id_src2->imm);

  if (ddest == dsrc1) {
    rtl_mv(s, s0, csr);
    rtl_or(s, csr, csr, dsrc1);
    rtl_mv(s, dsrc1, s0);
  } else {
    rtl_mv(s, ddest, csr);
    rtl_or(s, csr, csr, dsrc1);
  }

  print_asm_template3("csrrs");
}

void raise_intr(DecodeExecState *s, uint32_t NO, vaddr_t epc);
static inline make_EHelper(priv) {
  uint32_t type = s->isa.instr.csr.csr;
  switch (type) {
    case 0:
      raise_intr(s, 9, cpu.pc);
      print_asm("ecall");
      break;
    case 0x102:
      cpu.sstatus.sie = cpu.sstatus.spie;
      cpu.sstatus.spie = 1;
      rtl_j(s, cpu.sepc);
      print_asm("sret");
      break;
    default: panic("unimplemented priv instruction type = 0x%x", type);
  }

#ifndef __DIFF_REF_NEMU__
  difftest_skip_dut(1, 2);
#endif
}
