def_EHelper(lui) { rtl_li(s, ddest, id_src2->imm << 16); }
def_EHelper(add) { rtl_add(s, ddest, dsrc1, dsrc2); }
def_EHelper(sub) { rtl_sub(s, ddest, dsrc1, dsrc2); }
def_EHelper(slt) { rtl_setrelop(s, RELOP_LT, ddest, dsrc1, dsrc2); }
def_EHelper(sltu){ rtl_setrelop(s, RELOP_LTU, ddest, dsrc1, dsrc2); }
def_EHelper(and) { rtl_and(s, ddest, dsrc1, dsrc2); }
def_EHelper(or) { rtl_or(s, ddest, dsrc1, dsrc2); }
def_EHelper(xor) { rtl_xor(s, ddest, dsrc1, dsrc2); }
def_EHelper(nor) {
  rtl_or(s, ddest, dsrc1, dsrc2);
  rtl_not(s, ddest, ddest);
}
def_EHelper(sllv) { rtl_sll(s, ddest, dsrc2, dsrc1); }
def_EHelper(srlv) { rtl_srl(s, ddest, dsrc2, dsrc1); }
def_EHelper(srav) { rtl_sra(s, ddest, dsrc2, dsrc1); }
def_EHelper(addi) { rtl_addi(s, ddest, dsrc1, id_src2->imm); }
def_EHelper(slti) { rtl_setrelopi(s, RELOP_LT, ddest, dsrc1, id_src2->imm); }
def_EHelper(sltui){ rtl_setrelopi(s, RELOP_LTU, ddest, dsrc1, id_src2->imm); }
def_EHelper(andi) { rtl_andi(s, ddest, dsrc1, id_src2->imm); }
def_EHelper(ori)  { rtl_ori(s, ddest, dsrc1, id_src2->imm); }
def_EHelper(xori) { rtl_xori(s, ddest, dsrc1, id_src2->imm); }
def_EHelper(sll)  { rtl_slli(s, ddest, dsrc2, id_src1->imm); }
def_EHelper(srl)  { rtl_srli(s, ddest, dsrc2, id_src1->imm); }
def_EHelper(sra)  { rtl_srai(s, ddest, dsrc2, id_src1->imm); }
def_EHelper(movz) {
  rtl_setrelopi(s, RELOP_EQ, s0, dsrc2, 0);
  rtl_cmov(s, ddest, s0, dsrc1);
}
def_EHelper(movn) { rtl_cmov(s, ddest, dsrc2, dsrc1); }

def_EHelper(clz) {
#ifndef CONFIG_ENGINE_INTERPRETER
  panic("not support in engines other than interpreter");
#endif
  uint32_t clz(uint32_t x);
  *ddest = clz(*dsrc1);
}
