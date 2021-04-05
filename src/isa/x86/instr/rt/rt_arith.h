// dest <- sub result
static inline void cmp_internal(Decode *s, int width) {
  rtl_sub(s, s0, ddest, dsrc1);
  rtl_update_ZFSF(s, s0, width);
  rtl_is_sub_carry(s, s1, ddest, dsrc1);
  rtl_set_CF(s, s1);
  rtl_is_sub_overflow(s, s1, s0, ddest, dsrc1, width);
  rtl_set_OF(s, s1);
}

def_REHelper(sub) {
#ifdef LAZY_CC
  rtl_set_lazycc(s, ddest, dsrc1, NULL, LAZYCC_SUB, width);
  rtl_sub(s, ddest, ddest, dsrc1);
#else
  cmp_internal(s, width);
  rtl_mv(s, ddest, s0);
#endif
}
