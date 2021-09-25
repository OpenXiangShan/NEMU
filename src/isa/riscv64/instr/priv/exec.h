def_EHelper(system) {
  extern int rtl_sys_slow_path(Decode *s, rtlreg_t *dest, const rtlreg_t *src1, uint32_t id, rtlreg_t *jpc);
  int jmp = rtl_sys_slow_path(s, ddest, dsrc1, id_src2->imm, s0);
  if (jmp) rtl_priv_jr(s, s0);
  else rtl_priv_next(s);
}
