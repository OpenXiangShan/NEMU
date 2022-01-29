def_EHelper(mul)   { rtl_mulu_lo(s, ddest, dsrc1, dsrc2); }
def_EHelper(mulh)  { rtl_muls_hi(s, ddest, dsrc1, dsrc2); }
def_EHelper(mulhu) { rtl_mulu_hi(s, ddest, dsrc1, dsrc2); }
def_EHelper(mulw)  { rtl_mulw(s, ddest, dsrc1, dsrc2); }

def_EHelper(mulhsu) {
  // Algorithm:
  // We want to obtain ans = mulhsu(a, b).
  // Consider mulhu(a, b).
  // If a >= 0, then ans = mulhu(a, b);
  // If a = -x < 0, then a = 2^64 - x in two's complement
  // Then
  //   mulhu(a, b) = mulhu(2^64 -x , b) = ((2^64 - x)b) >> 64
  //               = ((2^64b) >> 64) + ((-xb) >> 64)
  //               = b + mulhsu(a, b) = b + ans
  // Therefore, ans = mulhu(a, b) - b
  //
  // In the end, ans = (a < 0 ? mulhu(a, b) - b : mulhu(a, b))
  //                 = mulhu(a, b) - (a < 0 ? b : 0)

  rtl_srai(s, s0, dsrc1, 63);
  rtl_and(s, s0, dsrc2, s0); // s0 = (id_src1->val < 0 ? id_src2->val : 0)
  rtl_mulu_hi(s, s1, dsrc1, dsrc2);
  rtl_sub(s, ddest, s1, s0);
}

def_EHelper(div) {
  rtl_divs_q(s, ddest, dsrc1, dsrc2);
}

def_EHelper(divu) {
  rtl_divu_q(s, ddest, dsrc1, dsrc2);
}

def_EHelper(rem) {
  rtl_divs_r(s, ddest, dsrc1, dsrc2);
}

def_EHelper(remu) {
  rtl_divu_r(s, ddest, dsrc1, dsrc2);
}

def_EHelper(divw) {
#ifdef CONFIG_ENGINE_INTERPRETER
  rtl_sext(s, s0, dsrc1, 4);
  rtl_sext(s, s1, dsrc2, 4);
  rtl_divs_q(s, s0, s0, s1);
  rtl_sext(s, ddest, s0, 4);
#else
  rtl_divw(s, ddest, dsrc1, dsrc2);
#endif
}

def_EHelper(remw) {
#ifdef CONFIG_ENGINE_INTERPRETER
  rtl_sext(s, s0, dsrc1, 4);
  rtl_sext(s, s1, dsrc2, 4);
  rtl_divs_r(s, s0, s0, s1);
  rtl_sext(s, ddest, s0, 4);
#else
  rtl_remw(s, ddest, dsrc1, dsrc2);
#endif
}

def_EHelper(divuw) {
#ifdef CONFIG_ENGINE_INTERPRETER
  rtl_zext(s, s0, dsrc1, 4);
  rtl_zext(s, s1, dsrc2, 4);
  rtl_divu_q(s, s0, s0, s1);
  rtl_sext(s, ddest, s0, 4);
#else
  rtl_divuw(s, ddest, dsrc1, dsrc2);
#endif
}

def_EHelper(remuw) {
#ifdef CONFIG_ENGINE_INTERPRETER
  rtl_zext(s, s0, dsrc1, 4);
  rtl_zext(s, s1, dsrc2, 4);
  rtl_divu_r(s, s0, s0, s1);
  rtl_sext(s, ddest, s0, 4);
#else
  rtl_remuw(s, ddest, dsrc1, dsrc2);
#endif
}
