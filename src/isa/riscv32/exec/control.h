#ifndef __ICS_EXPORT

def_EHelper(jal) {
  rtl_li(s, ddest, s->snpc);
  rtl_j(s, rs1);
  print_asm_template2(jal);
  eob();
}

def_EHelper(jalr) {
  rtl_addi(s, s0, dsrc1, rs2);
#ifdef __ENGINE_interpreter__
  rtl_andi(s, s0, s0, ~0x1u);
#endif
  rtl_jr(s, s0);

  rtl_li(s, ddest, s->snpc);

#ifndef __DIFF_REF_NEMU__
  difftest_skip_dut(1, 2);
#endif

  print_asm_template3(jalr);
  eob();
}

def_EHelper(beq) {
  rtl_jrelop(s, RELOP_EQ, dsrc1, dsrc2, rd);
  print_asm_template3(beq);
  eob();
}

def_EHelper(bne) {
  rtl_jrelop(s, RELOP_NE, dsrc1, dsrc2, rd);
  print_asm_template3(bne);
  eob();
}

def_EHelper(blt) {
  rtl_jrelop(s, RELOP_LT, dsrc1, dsrc2, rd);
  print_asm_template3(blt);
  eob();
}

def_EHelper(bge) {
  rtl_jrelop(s, RELOP_GE, dsrc1, dsrc2, rd);
  print_asm_template3(bge);
  eob();
}

def_EHelper(bltu) {
  rtl_jrelop(s, RELOP_LTU, dsrc1, dsrc2, rd);
  print_asm_template3(bltu);
  eob();
}

def_EHelper(bgeu) {
  rtl_jrelop(s, RELOP_GEU, dsrc1, dsrc2, rd);
  print_asm_template3(bgeu);
  eob();
}
#endif
