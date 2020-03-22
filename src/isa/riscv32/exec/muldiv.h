static inline make_EHelper(mul) {
  rtl_imul_lo(s, ddest, dsrc1, dsrc2);
  print_asm_template3(mul);
}

static inline make_EHelper(mulh) {
  rtl_imul_hi(s, ddest, dsrc1, dsrc2);
  print_asm_template3(mulh);
}

static inline make_EHelper(mulhu) {
  rtl_mul_hi(s, ddest, dsrc1, dsrc2);
  print_asm_template3(mulh);
}

static inline make_EHelper(div) {
  rtl_idiv_q(s, ddest, dsrc1, dsrc2);
  print_asm_template3(div);
}

static inline make_EHelper(divu) {
  rtl_div_q(s, ddest, dsrc1, dsrc2);
  print_asm_template3(divu);
}

static inline make_EHelper(rem) {
  rtl_idiv_r(s, ddest, dsrc1, dsrc2);
  print_asm_template3(rem);
}

static inline make_EHelper(remu) {
  rtl_div_r(s, ddest, dsrc1, dsrc2);
  print_asm_template3(remu);
}
