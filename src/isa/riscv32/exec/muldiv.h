static inline make_EHelper(mul) {
  rtl_imul_lo(s, s0, dsrc1, dsrc2);
  rtl_sr(s, id_dest->reg, s0, 4);

  print_asm_template3(mul);
}

static inline make_EHelper(mulh) {
  rtl_imul_hi(s, s0, dsrc1, dsrc2);
  rtl_sr(s, id_dest->reg, s0, 4);

  print_asm_template3(mulh);
}

static inline make_EHelper(mulhu) {
  rtl_mul_hi(s, s0, dsrc1, dsrc2);
  rtl_sr(s, id_dest->reg, s0, 4);

  print_asm_template3(mulh);
}

static inline make_EHelper(div) {
  rtl_idiv_q(s, s0, dsrc1, dsrc2);
  rtl_sr(s, id_dest->reg, s0, 4);

  print_asm_template3(div);
}

static inline make_EHelper(divu) {
  rtl_div_q(s, s0, dsrc1, dsrc2);
  rtl_sr(s, id_dest->reg, s0, 4);

  print_asm_template3(divu);
}

static inline make_EHelper(rem) {
  rtl_idiv_r(s, s0, dsrc1, dsrc2);
  rtl_sr(s, id_dest->reg, s0, 4);

  print_asm_template3(rem);
}

static inline make_EHelper(remu) {
  rtl_div_r(s, s0, dsrc1, dsrc2);
  rtl_sr(s, id_dest->reg, s0, 4);

  print_asm_template3(remu);
}
