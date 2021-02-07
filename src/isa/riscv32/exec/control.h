#ifndef __ICS_EXPORT

def_EHelper(jal) {
  rtl_li(s, ddest, s->snpc);
  rtl_j(s, id_src1->imm);
  print_asm_template2(jal);
}

def_EHelper(jalr) {
  rtl_addi(s, s0, dsrc1, id_src2->imm);
  IFDEF(CONFIG_ENGINE_INTERPRETER, rtl_andi(s, s0, s0, ~0x1u));
  rtl_jr(s, s0);
  rtl_li(s, ddest, s->snpc);
  IFUNDEF(CONFIG_DIFFTEST_REF_NEMU ,difftest_skip_dut(1, 2));
  print_asm_template3(jalr);
}

def_EHelper(beq) {
  rtl_jrelop(s, RELOP_EQ, dsrc1, dsrc2, id_dest->imm);
  print_asm_template3(beq);
}

def_EHelper(bne) {
  rtl_jrelop(s, RELOP_NE, dsrc1, dsrc2, id_dest->imm);
  print_asm_template3(bne);
}

def_EHelper(blt) {
  rtl_jrelop(s, RELOP_LT, dsrc1, dsrc2, id_dest->imm);
  print_asm_template3(blt);
}

def_EHelper(bge) {
  rtl_jrelop(s, RELOP_GE, dsrc1, dsrc2, id_dest->imm);
  print_asm_template3(bge);
}

def_EHelper(bltu) {
  rtl_jrelop(s, RELOP_LTU, dsrc1, dsrc2, id_dest->imm);
  print_asm_template3(bltu);
}

def_EHelper(bgeu) {
  rtl_jrelop(s, RELOP_GEU, dsrc1, dsrc2, id_dest->imm);
  print_asm_template3(bgeu);
}
#endif
