#ifndef __ICS_EXPORT
def_EHelper(csrrw) {
  IFUNDEF(CONFIG_DIFFTEST_REF_NEMU, difftest_skip_dut(1, 3));
  rtl_hostcall(s, HOSTCALL_CSR, ddest, dsrc1, id_src2->imm);
  print_asm_template3("csrrw");
}

def_EHelper(csrrs) {
  IFUNDEF(CONFIG_DIFFTEST_REF_NEMU, difftest_skip_dut(1, 3));
  rtl_hostcall(s, HOSTCALL_CSR, s0, NULL, id_src2->imm);
  rtl_or(s, s1, s0, dsrc1);
  rtl_mv(s, ddest, s0);
  rtl_hostcall(s, HOSTCALL_CSR, NULL, s1, id_src2->imm);
  print_asm_template3("csrrs");
}

def_EHelper(ecall) {
  IFUNDEF(CONFIG_DIFFTEST_REF_NEMU, difftest_skip_dut(1, 2));
  rtl_trap(s, thispc, 9);
  print_asm("ecall");
}

def_EHelper(sret) {
  IFUNDEF(CONFIG_DIFFTEST_REF_NEMU, difftest_skip_dut(1, 2));
  rtl_hostcall(s, HOSTCALL_PRIV, s0, NULL, 0x102);
  rtl_jr(s, s0);
  print_asm("sret");
}

def_EHelper(sfence_vma) {
  IFUNDEF(CONFIG_DIFFTEST_REF_NEMU, difftest_skip_dut(1, 2));
  rtl_hostcall(s, HOSTCALL_PRIV, NULL, NULL, 0x120);
  print_asm("sfence.vma");
}
#endif
