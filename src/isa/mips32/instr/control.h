#define difftest_skip_delay_slot() \
  IFUNDEF(CONFIG_DIFFTEST_REF_NEMU, difftest_skip_dut(2, 1))

def_EHelper(j) {
  difftest_skip_delay_slot();
  rtl_j(s, id_dest->imm);
}

def_EHelper(jal) {
  difftest_skip_delay_slot();
  word_t ra = s->pc + 8;
  rtl_j(s, id_dest->imm);
  rtl_li(s, &reg_l(31), ra);
}

def_EHelper(jr) {
  difftest_skip_delay_slot();
  rtl_jr(s, dsrc1);
}

def_EHelper(jalr) {
  difftest_skip_delay_slot();
  word_t ra = s->pc + 8;
  rtl_jr(s, dsrc1);
  rtl_li(s, ddest, ra);
}

def_EHelper(bne) {
  difftest_skip_delay_slot();
  rtl_jrelop(s, RELOP_NE, dsrc1, dsrc2, id_dest->imm);
}

def_EHelper(beq) {
  difftest_skip_delay_slot();
  rtl_jrelop(s, RELOP_EQ, dsrc1, dsrc2, id_dest->imm);
}

def_EHelper(blez) {
  difftest_skip_delay_slot();
  rtl_jrelop(s, RELOP_LE, dsrc1, dsrc2, id_dest->imm);
}

def_EHelper(bltz) {
  difftest_skip_delay_slot();
  rtl_jrelop(s, RELOP_LT, dsrc1, rz, id_dest->imm);
}

def_EHelper(bgtz) {
  difftest_skip_delay_slot();
  rtl_jrelop(s, RELOP_GT, dsrc1, dsrc2, id_dest->imm);
}

def_EHelper(bgez) {
  difftest_skip_delay_slot();
  rtl_jrelop(s, RELOP_GE, dsrc1, rz, id_dest->imm);
}
