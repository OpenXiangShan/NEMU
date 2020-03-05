static inline make_EHelper(lui) {
  rtl_shli(s, s0, dsrc2, 16);
  rtl_sr(s, id_dest->reg, s0, 4);

  print_asm_template3(lui);
}

static inline make_EHelper(add) {
  rtl_add(s, s0, dsrc1, dsrc2);
  rtl_sr(s, id_dest->reg, s0, 4);

  print_asm_template3(add);
}

static inline make_EHelper(sub) {
  rtl_sub(s, s0, dsrc1, dsrc2);
  rtl_sr(s, id_dest->reg, s0, 4);

  print_asm_template3(sub);
}

static inline make_EHelper(slt) {
  rtl_setrelop(s, RELOP_LT, s0, dsrc1, dsrc2);
  rtl_sr(s, id_dest->reg, s0, 4);

  print_asm_template3(slt);
}

static inline make_EHelper(sltu) {
  rtl_setrelop(s, RELOP_LTU, s0, dsrc1, dsrc2);
  rtl_sr(s, id_dest->reg, s0, 4);

  print_asm_template3(sltu);
}

static inline make_EHelper(and) {
  rtl_and(s, s0, dsrc1, dsrc2);
  rtl_sr(s, id_dest->reg, s0, 4);

  print_asm_template3(and);
}

static inline make_EHelper(or) {
  rtl_or(s, s0, dsrc1, dsrc2);
  rtl_sr(s, id_dest->reg, s0, 4);

  print_asm_template3(or);
}

static inline make_EHelper(xor) {
  rtl_xor(s, s0, dsrc1, dsrc2);
  rtl_sr(s, id_dest->reg, s0, 4);

  print_asm_template3(xor);
}

static inline make_EHelper(nor) {
  rtl_or(s, s0, dsrc1, dsrc2);
  rtl_not(s, s0, s0);
  rtl_sr(s, id_dest->reg, s0, 4);

  print_asm_template3(nor);
}

static inline make_EHelper(sll) {
  rtl_shl(s, s0, dsrc2, dsrc1);
  rtl_sr(s, id_dest->reg, s0, 4);

  print_asm_template3(sll);
}

static inline make_EHelper(srl) {
  rtl_shr(s, s0, dsrc2, dsrc1);
  rtl_sr(s, id_dest->reg, s0, 4);

  print_asm_template3(srl);
}

static inline make_EHelper(sra) {
  rtl_sar(s, s0, dsrc2, dsrc1);
  rtl_sr(s, id_dest->reg, s0, 4);

  print_asm_template3(sra);
}

static inline make_EHelper(movz) {
  rtl_mux(s, s0, dsrc2, ddest, dsrc1);
  rtl_sr(s, id_dest->reg, s0, 4);

  print_asm_template3(movz);
}

static inline make_EHelper(movn) {
  rtl_mux(s, s0, dsrc2, dsrc1, ddest);
  rtl_sr(s, id_dest->reg, s0, 4);

  print_asm_template3(movn);
}
