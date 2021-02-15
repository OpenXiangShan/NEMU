def_EHelper(mfhi) {
  rtl_mv(s, ddest, &cpu.hi);
}

def_EHelper(mflo) {
  rtl_mv(s, ddest, &cpu.lo);
}

def_EHelper(mthi) {
  rtl_mv(s, &cpu.hi, dsrc1);
}

def_EHelper(mtlo) {
  rtl_mv(s, &cpu.lo, dsrc1);
}

def_EHelper(mul) {
  rtl_mulu_lo(s, ddest, dsrc1, dsrc2);
}

def_EHelper(mult) {
  rtl_mulu_lo(s, &cpu.lo, dsrc1, dsrc2);
  rtl_muls_hi(s, &cpu.hi, dsrc1, dsrc2);
}

def_EHelper(multu) {
  rtl_mulu_lo(s, &cpu.lo, dsrc1, dsrc2);
  rtl_mulu_hi(s, &cpu.hi, dsrc1, dsrc2);
}

def_EHelper(div) {
  rtl_divs_q(s, &cpu.lo, dsrc1, dsrc2);
  rtl_divs_r(s, &cpu.hi, dsrc1, dsrc2);
}

def_EHelper(divu) {
  rtl_divu_q(s, &cpu.lo, dsrc1, dsrc2);
  rtl_divu_r(s, &cpu.hi, dsrc1, dsrc2);
}
