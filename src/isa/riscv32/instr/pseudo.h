def_EHelper(li_0) {
  rtl_li(s, ddest, 0);
}

def_EHelper(li_1) {
  rtl_li(s, ddest, 1);
}

def_EHelper(li) {
  rtl_li(s, ddest, id_src2->imm);
}

def_EHelper(mv) {
  rtl_mv(s, ddest, dsrc1);
}

def_EHelper(beqz) {
  rtl_jrelop(s, RELOP_EQ, dsrc1, rz, id_dest->imm);
}

def_EHelper(bnez) {
  rtl_jrelop(s, RELOP_NE, dsrc1, rz, id_dest->imm);
}

def_EHelper(blez) {
  rtl_jrelop(s, RELOP_GE, rz, dsrc2, id_dest->imm);
}

def_EHelper(bgtz) {
  rtl_jrelop(s, RELOP_LT, rz, dsrc2, id_dest->imm);
}

def_EHelper(bltz) {
  rtl_jrelop(s, RELOP_LT, dsrc1, rz, id_dest->imm);
}

def_EHelper(bgez) {
  rtl_jrelop(s, RELOP_GE, dsrc1, rz, id_dest->imm);
}

def_EHelper(j) {
  rtl_j(s, id_src1->imm);
}

def_EHelper(jal_ra) {
  rtl_li(s, &cpu.gpr[1]._32, id_src2->imm);
  rtl_j(s, id_src1->imm);
}

def_EHelper(ret) {
//  IFDEF(CONFIG_ENGINE_INTERPRETER, rtl_andi(s, s0, s0, ~0x1u));
  IFNDEF(CONFIG_DIFFTEST_REF_NEMU, difftest_skip_dut(1, 2));
  rtl_jr(s, &cpu.gpr[1]._32);
}

def_EHelper(jr) {
//  IFDEF(CONFIG_ENGINE_INTERPRETER, rtl_andi(s, s0, s0, ~0x1u));
  IFNDEF(CONFIG_DIFFTEST_REF_NEMU, difftest_skip_dut(1, 2));
  rtl_jr(s, dsrc1);
}

def_EHelper(jr_imm) {
  rtl_addi(s, s0, dsrc1, id_src2->imm);
//  IFDEF(CONFIG_ENGINE_INTERPRETER, rtl_andi(s, s0, s0, ~0x1u));
  IFNDEF(CONFIG_DIFFTEST_REF_NEMU, difftest_skip_dut(1, 2));
  rtl_jr(s, s0);
}
