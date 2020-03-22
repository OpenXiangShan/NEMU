static inline make_EHelper(mfhi) {
  rtl_mv(s, ddest, &cpu.hi);
  print_asm_template3(mfhi);
}

static inline make_EHelper(mflo) {
  rtl_mv(s, ddest, &cpu.lo);
  print_asm_template3(mflo);
}

static inline make_EHelper(mthi) {
  rtl_mv(s, &cpu.hi, dsrc1);
  print_asm_template2(mthi);
}

static inline make_EHelper(mtlo) {
  rtl_mv(s, &cpu.lo, dsrc1);
  print_asm_template2(mtlo);
}

static inline make_EHelper(mul) {
  rtl_imul_lo(s, ddest, dsrc1, dsrc2);
  print_asm_template3(mul);
}

static inline make_EHelper(mult) {
  rtl_imul_lo(s, &cpu.lo, dsrc1, dsrc2);
  rtl_imul_hi(s, &cpu.hi, dsrc1, dsrc2);
  print_asm_template3(mult);
}

static inline make_EHelper(multu) {
  rtl_mul_lo(s, &cpu.lo, dsrc1, dsrc2);
  rtl_mul_hi(s, &cpu.hi, dsrc1, dsrc2);
  print_asm_template3(multu);
}

static inline make_EHelper(div) {
  rtl_idiv_q(s, &cpu.lo, dsrc1, dsrc2);
  rtl_idiv_r(s, &cpu.hi, dsrc1, dsrc2);
  print_asm_template3(div);
}

static inline make_EHelper(divu) {
  rtl_div_q(s, &cpu.lo, dsrc1, dsrc2);
  rtl_div_r(s, &cpu.hi, dsrc1, dsrc2);
  print_asm_template3(divu);
}
