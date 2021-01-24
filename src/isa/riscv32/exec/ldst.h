static inline def_EHelper(lw) {
  rtl_lms(s, ddest, dsrc1, id_src2->imm, 4);
  print_Dop(id_src1->str, OP_STR_SIZE, "%d(%s)", id_src2->imm, reg_name(id_src1->reg, 4));
  print_asm_template2(lw);
}

static inline def_EHelper(sw) {
  rtl_sm(s, dsrc1, id_src2->imm, ddest, 4);
  print_Dop(id_src1->str, OP_STR_SIZE, "%d(%s)", id_src2->imm, reg_name(id_src1->reg, 4));
  print_asm_template2(sw);
}

#ifndef __ICS_EXPORT
static inline def_EHelper(lh) {
  rtl_lms(s, ddest, dsrc1, id_src2->imm, 2);
  print_Dop(id_src1->str, OP_STR_SIZE, "%d(%s)", id_src2->imm, reg_name(id_src1->reg, 4));
  print_asm_template2(lh);
}

static inline def_EHelper(lb) {
  rtl_lms(s, ddest, dsrc1, id_src2->imm, 1);
  print_Dop(id_src1->str, OP_STR_SIZE, "%d(%s)", id_src2->imm, reg_name(id_src1->reg, 4));
  print_asm_template2(lb);
}

static inline def_EHelper(lhu) {
  rtl_lm(s, ddest, dsrc1, id_src2->imm, 2);
  print_Dop(id_src1->str, OP_STR_SIZE, "%d(%s)", id_src2->imm, reg_name(id_src1->reg, 4));
  print_asm_template2(lhu);
}

static inline def_EHelper(lbu) {
  rtl_lm(s, ddest, dsrc1, id_src2->imm, 1);
  print_Dop(id_src1->str, OP_STR_SIZE, "%d(%s)", id_src2->imm, reg_name(id_src1->reg, 4));
  print_asm_template2(lbu);
}

static inline def_EHelper(sh) {
  rtl_sm(s, dsrc1, id_src2->imm, ddest, 2);
  print_Dop(id_src1->str, OP_STR_SIZE, "%d(%s)", id_src2->imm, reg_name(id_src1->reg, 4));
  print_asm_template2(sh);
}

static inline def_EHelper(sb) {
  rtl_sm(s, dsrc1, id_src2->imm, ddest, 1);
  print_Dop(id_src1->str, OP_STR_SIZE, "%d(%s)", id_src2->imm, reg_name(id_src1->reg, 4));
  print_asm_template2(sb);
}
#endif
