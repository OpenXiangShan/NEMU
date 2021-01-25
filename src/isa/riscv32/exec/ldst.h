def_EHelper(lw) {
  rtl_lms(s, ddest, dsrc1, id_src2->imm, 4);
  print_Dop(id_src1->str, OP_STR_SIZE, "%d(%s)", id_src2->imm, reg_name(id_src1->reg, 4));
  print_asm_template2(lw);
}

def_EHelper(sw) {
  rtl_sm(s, dsrc1, id_src2->imm, ddest, 4);
  print_Dop(id_src1->str, OP_STR_SIZE, "%d(%s)", id_src2->imm, reg_name(id_src1->reg, 4));
  print_asm_template2(sw);
}

#ifndef __ICS_EXPORT
def_EHelper(lh) {
  rtl_lms(s, ddest, dsrc1, id_src2->imm, 2);
  print_Dop(id_src1->str, OP_STR_SIZE, "%d(%s)", id_src2->imm, reg_name(id_src1->reg, 4));
  print_asm_template2(lh);
}

def_EHelper(lb) {
  rtl_lms(s, ddest, dsrc1, id_src2->imm, 1);
  print_Dop(id_src1->str, OP_STR_SIZE, "%d(%s)", id_src2->imm, reg_name(id_src1->reg, 4));
  print_asm_template2(lb);
}

def_EHelper(lhu) {
  rtl_lm(s, ddest, dsrc1, id_src2->imm, 2);
  print_Dop(id_src1->str, OP_STR_SIZE, "%d(%s)", id_src2->imm, reg_name(id_src1->reg, 4));
  print_asm_template2(lhu);
}

def_EHelper(lbu) {
  rtl_lm(s, ddest, dsrc1, id_src2->imm, 1);
  print_Dop(id_src1->str, OP_STR_SIZE, "%d(%s)", id_src2->imm, reg_name(id_src1->reg, 4));
  print_asm_template2(lbu);
}

def_EHelper(sh) {
  rtl_sm(s, dsrc1, id_src2->imm, ddest, 2);
  print_Dop(id_src1->str, OP_STR_SIZE, "%d(%s)", id_src2->imm, reg_name(id_src1->reg, 4));
  print_asm_template2(sh);
}

def_EHelper(sb) {
  rtl_sm(s, dsrc1, id_src2->imm, ddest, 1);
  print_Dop(id_src1->str, OP_STR_SIZE, "%d(%s)", id_src2->imm, reg_name(id_src1->reg, 4));
  print_asm_template2(sb);
}
#endif
