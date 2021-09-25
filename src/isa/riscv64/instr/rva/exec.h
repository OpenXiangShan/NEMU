def_EHelper(atomic) {
  extern void rtl_amo_slow_path(Decode *s, rtlreg_t *dest, const rtlreg_t *src1, const rtlreg_t *src2);
  rtl_amo_slow_path(s, ddest, dsrc1, dsrc2);
}
