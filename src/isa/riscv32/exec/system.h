static inline rtlreg_t* csr_decode(uint32_t csr) {
  difftest_skip_dut(1, 3);

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
  rtlreg_t *csr = csr_decode(id_src2->val);

  rtl_sr(s, id_dest->reg, csr, 4);
  rtl_mv(s, csr, dsrc1);

  print_asm_template3("csrrw");
}

static inline make_EHelper(csrrs) {
  rtlreg_t *csr = csr_decode(id_src2->val);

  rtl_sr(s, id_dest->reg, csr, 4);
  rtl_or(s, csr, csr, dsrc1);

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
      rtl_li(s, s0, cpu.sepc);
      rtl_jr(s, s0);
      print_asm("sret");
      break;
    default: panic("unimplemented priv instruction type = 0x%x", type);
  }

  difftest_skip_dut(1, 2);
}
