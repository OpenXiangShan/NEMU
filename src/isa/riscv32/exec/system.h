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

def_EHelper(ecall) {
#ifndef __DIFF_REF_NEMU__
  difftest_skip_dut(1, 2);
#endif
  rtl_trap(s, thispc, 9);
  print_asm("ecall");
}

def_EHelper(sret) {
#ifndef __DIFF_REF_NEMU__
  difftest_skip_dut(1, 2);
#endif
  rtl_hostcall(s, HOSTCALL_PRIV, s0, NULL, 0x102);
  rtl_jr(s, s0);
  print_asm("sret");
}

def_EHelper(sfence_vma) {
#ifndef __DIFF_REF_NEMU__
  difftest_skip_dut(1, 2);
#endif
  rtl_hostcall(s, HOSTCALL_PRIV, NULL, NULL, 0x120);
  print_asm("sfence.vma");
}
#endif
