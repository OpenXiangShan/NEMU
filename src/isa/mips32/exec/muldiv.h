#ifndef __ICS_EXPORT
static inline def_EHelper(mfhi) {
  rtl_mv(s, ddest, &cpu.hi);
  print_asm_template3(mfhi);
}

static inline def_EHelper(mflo) {
  rtl_mv(s, ddest, &cpu.lo);
  print_asm_template3(mflo);
}

static inline def_EHelper(mthi) {
  rtl_mv(s, &cpu.hi, dsrc1);
  print_asm_template2(mthi);
}

static inline def_EHelper(mtlo) {
  rtl_mv(s, &cpu.lo, dsrc1);
  print_asm_template2(mtlo);
}

static inline def_EHelper(mul) {
  rtl_mulu_lo(s, ddest, dsrc1, dsrc2);
  print_asm_template3(mul);
}

static inline def_EHelper(mult) {
  rtl_mulu_lo(s, &cpu.lo, dsrc1, dsrc2);
  rtl_muls_hi(s, &cpu.hi, dsrc1, dsrc2);
  print_asm_template3(mult);
}

static inline def_EHelper(multu) {
  rtl_mulu_lo(s, &cpu.lo, dsrc1, dsrc2);
  rtl_mulu_hi(s, &cpu.hi, dsrc1, dsrc2);
  print_asm_template3(multu);
}

static inline def_EHelper(div) {
  rtl_divs_q(s, &cpu.lo, dsrc1, dsrc2);
  rtl_divs_r(s, &cpu.hi, dsrc1, dsrc2);
  print_asm_template3(div);
}

static inline def_EHelper(divu) {
  rtl_divu_q(s, &cpu.lo, dsrc1, dsrc2);
  rtl_divu_r(s, &cpu.hi, dsrc1, dsrc2);
  print_asm_template3(divu);
}
#endif
