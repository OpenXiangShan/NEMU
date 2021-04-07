def_REHelper(add) {
#ifdef LAZY_CC
  rtl_set_lazycc_src1(s, dsrc1);  // set src firstly cuz maybe $dest = $src
  rtl_add(s, ddest, ddest, dsrc1);
  rtl_set_lazycc(s, ddest, NULL, NULL, LAZYCC_ADD, width);
#else
  rtl_add(s, s0, ddest, dsrc1);
  rtl_update_ZFSF(s, s0, width);
  if (width != 4) {
    rtl_andi(s, s0, s0, 0xffffffffu >> ((4 - width) * 8));
  }
  rtl_is_add_carry(s, s1, s0, ddest);
  rtl_set_CF(s, s1);
  rtl_is_add_overflow(s, s1, s0, ddest, dsrc1, width);
  rtl_set_OF(s, s1);
  rtl_mv(s, ddest, s0);
#endif
}

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

def_REHelper(cmp) {
#ifdef LAZY_CC
  rtl_set_lazycc(s, ddest, dsrc1, NULL, LAZYCC_SUB, width);
#else
  cmp_internal(s, width);
#endif
}

def_REHelper(inc) {
  rtl_addi(s, ddest, ddest, 1);
#ifdef LAZY_CC
  rtl_set_lazycc(s, ddest, NULL, NULL, LAZYCC_INC, width);
#else
  rtl_update_ZFSF(s, ddest, width);
  rtl_setrelopi(s, RELOP_EQ, s1, ddest, 0x1u << (width * 8 - 1));
  rtl_set_OF(s, s1);
#endif
}
