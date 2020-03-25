static inline make_EHelper(lui) {
  rtl_li(s, ddest, id_src2->imm << 16);
  print_asm_template3(lui);
}

static inline make_EHelper(add) {
  rtl_add(s, ddest, dsrc1, dsrc2);
  print_asm_template3(add);
}

static inline make_EHelper(sub) {
  rtl_sub(s, ddest, dsrc1, dsrc2);
  print_asm_template3(sub);
}

static inline make_EHelper(slt) {
  rtl_setrelop(s, RELOP_LT, ddest, dsrc1, dsrc2);
  print_asm_template3(slt);
}

static inline make_EHelper(sltu) {
  rtl_setrelop(s, RELOP_LTU, ddest, dsrc1, dsrc2);
  print_asm_template3(sltu);
}

static inline make_EHelper(and) {
  rtl_and(s, ddest, dsrc1, dsrc2);
  print_asm_template3(and);
}

static inline make_EHelper(or) {
  rtl_or(s, ddest, dsrc1, dsrc2);
  print_asm_template3(or);
}

static inline make_EHelper(xor) {
  rtl_xor(s, ddest, dsrc1, dsrc2);
  print_asm_template3(xor);
}

static inline make_EHelper(nor) {
  rtl_or(s, ddest, dsrc1, dsrc2);
  rtl_not(s, ddest, ddest);
  print_asm_template3(nor);
}

static inline make_EHelper(sll) {
  rtl_shl(s, ddest, dsrc2, dsrc1);
  print_asm_template3(sll);
}

static inline make_EHelper(srl) {
  rtl_shr(s, ddest, dsrc2, dsrc1);
  print_asm_template3(srl);
}

static inline make_EHelper(sra) {
  rtl_sar(s, ddest, dsrc2, dsrc1);
  print_asm_template3(sra);
}

static inline make_EHelper(addi) {
  rtl_addi(s, ddest, dsrc1, id_src2->imm);
  print_asm_template3(addi);
}

static inline make_EHelper(slti) {
  rtl_setrelopi(s, RELOP_LT, ddest, dsrc1, id_src2->imm);
  print_asm_template3(slti);
}

static inline make_EHelper(sltui) {
  rtl_setrelopi(s, RELOP_LTU, ddest, dsrc1, id_src2->imm);
  print_asm_template3(sltui);
}

static inline make_EHelper(andi) {
  rtl_andi(s, ddest, dsrc1, id_src2->imm);
  print_asm_template3(andi);
}

static inline make_EHelper(ori) {
  rtl_ori(s, ddest, dsrc1, id_src2->imm);
  print_asm_template3(ori);
}

static inline make_EHelper(xori) {
  rtl_xori(s, ddest, dsrc1, id_src2->imm);
  print_asm_template3(xori);
}

static inline make_EHelper(slli) {
  rtl_shli(s, ddest, dsrc2, id_src1->imm);
  print_asm_template3(slli);
}

static inline make_EHelper(srli) {
  rtl_shri(s, ddest, dsrc2, id_src1->imm);
  print_asm_template3(srli);
}

static inline make_EHelper(srai) {
  rtl_sari(s, ddest, dsrc2, id_src1->imm);
  print_asm_template3(srai);
}

static inline void mux(DecodeExecState *s, rtlreg_t* dest, const rtlreg_t* cond,
    const rtlreg_t* src1, const rtlreg_t* src2) {
  // dest <- (cond ? src1 : src2)
  rtl_setrelopi(s, RELOP_EQ, s0, cond, 0);
  rtl_subi(s, s0, s0, 1);
  // s0 = mask
  rtl_and(s, s1, src1, s0);
  rtl_not(s, s0, s0);
  rtl_and(s, dest, src2, s0);
  rtl_or(s, dest, dest, s1);
}

static inline make_EHelper(movz) {
  mux(s, ddest, dsrc2, ddest, dsrc1);
  print_asm_template3(movz);
}

static inline make_EHelper(movn) {
  mux(s, ddest, dsrc2, dsrc1, ddest);
  print_asm_template3(movn);
}
