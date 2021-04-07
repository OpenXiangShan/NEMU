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

def_REHelper(dec) {
#ifdef LAZY_CC
  rtl_subi(s, ddest, ddest, 1);
  rtl_set_lazycc(s, ddest, NULL, NULL, LAZYCC_DEC, width);
#else
  rtl_subi(s, s0, ddest, 1);
  rtl_update_ZFSF(s, s0, width);
  rtl_setrelopi(s, RELOP_EQ, s1, ddest, 0x1u << (width * 8 - 1));
  rtl_set_OF(s, s1);
  rtl_mv(s, ddest, s0);
#endif
}

def_REHelper(adc) {
#ifdef LAZY_CC
  rtl_lazy_setcc(s, s0, CC_B); // reading CC_B is to read CF
  rtl_add(s, s0, dsrc1, s0);
  rtl_set_lazycc_src2(s, dsrc1);
  rtl_add(s, ddest, ddest, s0);
  rtl_set_lazycc(s, ddest, s0, NULL, LAZYCC_ADC, width);
#else
  rtl_get_CF(s, s0);
  rtl_add(s, s0, dsrc1, s0);
  rtl_add(s, s1, ddest, s0);
  rtl_update_ZFSF(s, s1, width);
  rtl_is_add_overflow(s, s2, s1, ddest, dsrc1, width);
  rtl_set_OF(s, s2);
  if (width != 4) {
    rtl_andi(s, s1, s1, 0xffffffffu >> ((4 - width) * 8));
  }
  rtl_is_add_carry(s, s2, s1, s0);
  rtl_is_add_carry(s, s0, s0, dsrc1);
  rtl_or(s, s0, s0, s2);
  rtl_set_CF(s, s0);
  rtl_mv(s, ddest, s1);
#endif
}

def_REHelper(sbb) {
#ifdef LAZY_CC
  rtl_lazy_setcc(s, s0, CC_B); // reading CC_B is to read CF
  rtl_add(s, s0, dsrc1, s0);
  rtl_set_lazycc_src2(s, dsrc1);
  rtl_set_lazycc_src1(s, ddest);
  rtl_sub(s, ddest, ddest, s0);
  rtl_set_lazycc(s, ddest, NULL, NULL, LAZYCC_SBB, width);
#else
  rtl_get_CF(s, s0);
  rtl_add(s, s0, dsrc1, s0);
  rtl_sub(s, s1, ddest, s0);
  rtl_update_ZFSF(s, s1, width);
  rtl_is_sub_overflow(s, s2, s1, ddest, dsrc1, width);
  rtl_set_OF(s, s2);
  rtl_is_add_carry(s, s2, s0, dsrc1);
  rtl_is_sub_carry(s, s0, ddest, s0);
  rtl_or(s, s0, s0, s2);
  rtl_set_CF(s, s0);
  rtl_mv(s, ddest, s1);
#endif
}



// imul with two operands
def_REHelper(imul2) {
  rtl_sext(s, dsrc1, dsrc1, width);
  rtl_sext(s, ddest, ddest, width);

#ifndef __PA__
  if (width == 4) { rtl_muls_hi(s, s1, ddest, dsrc1); }
#endif

  rtl_mulu_lo(s, ddest, ddest, dsrc1);

#ifndef __PA__
  if (width == 2) {
    rtl_sext(s, s0, ddest, width);
    rtl_setrelop(s, RELOP_NE, s0, s0, ddest);
  } else if (width == 4) {
    rtl_msb(s, s0, ddest, width);
    rtl_add(s, s0, s1, s0);
    rtl_setrelopi(s, RELOP_NE, s0, s0, 0);
  } else {
    assert(0);
  }
  rtl_set_CF(s, s0);
  rtl_set_OF(s, s0);
  rtl_update_ZFSF(s, ddest, width);
#endif
}

def_REHelper(idiv) {
  switch (width) {
    case 1:
      rtl_lr(s, s0, R_AX, 2);
      rtl_divs_q(s, s1, s0, ddest);
      rtl_divs_r(s, s0, s0, ddest);
      rtl_sr(s, R_AL, s1, 1);
      rtl_sr(s, R_AH, s0, 1);
      break;
    case 2:
      rtl_lr(s, s0, R_AX, 2);
      rtl_lr(s, s1, R_DX, 2);
      rtl_shli(s, s1, s1, 16);
      rtl_or(s, s0, s0, s1);
      rtl_divs_q(s, s1, s0, ddest);
      rtl_divs_r(s, s0, s0, ddest);
      rtl_sr(s, R_AX, s1, 2);
      rtl_sr(s, R_DX, s0, 2);
      break;
    case 4:
      ; rtlreg_t *pdest = ddest;
      if (ddest == &cpu.eax) pdest = s0;
      rtl_mv(s, s0, &cpu.eax);
      rtl_div64s_q(s, &cpu.eax, &cpu.edx, s0, pdest);
      rtl_div64s_r(s, &cpu.edx, &cpu.edx, s0, pdest);
      break;
    default: assert(0);
  }
}
