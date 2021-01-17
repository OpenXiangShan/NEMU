#ifndef __ICS_EXPORT
static inline def_EHelper(csrrw) {
#ifndef __DIFF_REF_NEMU__
  difftest_skip_dut(1, 3);
#endif
  rtl_hostcall(s, HOSTCALL_CSR, ddest, dsrc1, id_src2->imm);
  print_asm_template3("csrrw");
}

static inline def_EHelper(csrrs) {
#ifndef __DIFF_REF_NEMU__
  difftest_skip_dut(1, 3);
#endif
  rtl_hostcall(s, HOSTCALL_CSR, s0, NULL, id_src2->imm);
  rtl_or(s, s1, s0, dsrc1);
  rtl_mv(s, ddest, s0);
  rtl_hostcall(s, HOSTCALL_CSR, NULL, s1, id_src2->imm);
  print_asm_template3("csrrs");
}

static inline def_EHelper(priv) {
  uint32_t type = s->isa.instr.csr.csr;
  switch (type) {
    case 0:
      rtl_li(s, s0, cpu.pc);
      rtl_hostcall(s, HOSTCALL_TRAP, s0, s0, 9);
      rtl_jr(s, s0);
      print_asm("ecall");
      break;
    case 0x102:
      rtl_hostcall(s, HOSTCALL_PRIV, s0, NULL, 0x102);
      rtl_jr(s, s0);
      print_asm("sret");
      break;
    default: panic("unimplemented priv instruction type = 0x%x", type);
  }

#ifndef __DIFF_REF_NEMU__
  difftest_skip_dut(1, 2);
#endif
}
#endif
