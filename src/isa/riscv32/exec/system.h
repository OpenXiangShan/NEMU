#ifndef __ICS_EXPORT
def_EHelper(csrrw) {
#ifndef __DIFF_REF_NEMU__
  difftest_skip_dut(1, 3);
#endif
  rtl_hostcall(s, HOSTCALL_CSR, ddest, dsrc1, rs2);
  print_asm_template3("csrrw");
}

def_EHelper(csrrs) {
#ifndef __DIFF_REF_NEMU__
  difftest_skip_dut(1, 3);
#endif
  rtl_hostcall(s, HOSTCALL_CSR, s0, NULL, rs2);
  rtl_or(s, s1, s0, dsrc1);
  rtl_mv(s, ddest, s0);
  rtl_hostcall(s, HOSTCALL_CSR, NULL, s1, rs2);
  print_asm_template3("csrrs");
}

def_EHelper(priv) {
  uint32_t type = s->isa.instr.csr.csr;
  switch (type) {
    case 0:
      rtl_trap(s, cpu.pc, 9);
      print_asm("ecall");
      break;
    case 0x102:
      rtl_hostcall(s, HOSTCALL_PRIV, s0, NULL, type);
      rtl_jr(s, s0);
      print_asm("sret");
      break;
    case 0x120:
      rtl_hostcall(s, HOSTCALL_PRIV, NULL, NULL, type);
      print_asm("sfence.vma");
      break;
    default: panic("unimplemented priv instruction type = 0x%x", type);
  }

#ifndef __DIFF_REF_NEMU__
  difftest_skip_dut(1, 2);
#endif
}
#endif
