def_EHelper(lw) {
  update_gpc(lpc);
  rtl_lms(s, ddest, dsrc1, rs2, 4);
  print_Dop(id_src1->str, OP_STR_SIZE, "%d(%s)", rs2, reg_name(id_src1->reg, 4));
  print_asm_template2(lw);
}

def_EHelper(sw) {
  update_gpc(lpc);
  rtl_sm(s, dsrc1, rs2, ddest, 4);
  print_Dop(id_src1->str, OP_STR_SIZE, "%d(%s)", rs2, reg_name(id_src1->reg, 4));
  print_asm_template2(sw);
}

#ifndef __ICS_EXPORT
def_EHelper(lh) {
  update_gpc(lpc);
  rtl_lms(s, ddest, dsrc1, rs2, 2);
  print_Dop(id_src1->str, OP_STR_SIZE, "%d(%s)", rs2, reg_name(id_src1->reg, 4));
  print_asm_template2(lh);
}

def_EHelper(lb) {
  update_gpc(lpc);
  rtl_lms(s, ddest, dsrc1, rs2, 1);
  print_Dop(id_src1->str, OP_STR_SIZE, "%d(%s)", rs2, reg_name(id_src1->reg, 4));
  print_asm_template2(lb);
}

def_EHelper(lhu) {
  update_gpc(lpc);
  rtl_lm(s, ddest, dsrc1, rs2, 2);
  print_Dop(id_src1->str, OP_STR_SIZE, "%d(%s)", rs2, reg_name(id_src1->reg, 4));
  print_asm_template2(lhu);
}

def_EHelper(lbu) {
  update_gpc(lpc);
  rtl_lm(s, ddest, dsrc1, rs2, 1);
  print_Dop(id_src1->str, OP_STR_SIZE, "%d(%s)", rs2, reg_name(id_src1->reg, 4));
  print_asm_template2(lbu);
}

def_EHelper(sh) {
  update_gpc(lpc);
  rtl_sm(s, dsrc1, rs2, ddest, 2);
  print_Dop(id_src1->str, OP_STR_SIZE, "%d(%s)", rs2, reg_name(id_src1->reg, 4));
  print_asm_template2(sh);
}

def_EHelper(sb) {
  update_gpc(lpc);
  rtl_sm(s, dsrc1, rs2, ddest, 1);
  print_Dop(id_src1->str, OP_STR_SIZE, "%d(%s)", rs2, reg_name(id_src1->reg, 4));
  print_asm_template2(sb);
}
#endif
