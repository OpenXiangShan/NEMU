def_EHelper(fence_i) {
  IFNDEF(CONFIG_DIFFTEST_REF_NEMU, difftest_skip_dut(1, 2));
  rtl_hostcall(s, HOSTCALL_PRIV, NULL, NULL, NULL, -1);
  rtl_priv_next(s);
}

def_EHelper(fence) {
  IFNDEF(CONFIG_DIFFTEST_REF_NEMU, difftest_skip_dut(1, 2));
}
