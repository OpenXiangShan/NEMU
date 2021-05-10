def_EHelper(add) {
  rtl_add(s, ddest, dsrc1, dsrc2);
}

def_EHelper(sub) {
  rtl_sub(s, ddest, dsrc1, dsrc2);
}

def_EHelper(sll) {
  rtl_shl(s, ddest, dsrc1, dsrc2);
}

def_EHelper(sra) {
  rtl_sar(s, ddest, dsrc1, dsrc2);
}

def_EHelper(srl) {
  rtl_shr(s, ddest, dsrc1, dsrc2);
}

def_EHelper(slt) {
  rtl_setrelop(s, RELOP_LT, ddest, dsrc1, dsrc2);
}

def_EHelper(sltu) {
  rtl_setrelop(s, RELOP_LTU, ddest, dsrc1, dsrc2);
}

def_EHelper(nemuxor) {
  rtl_nemuxor(s, ddest, dsrc1, dsrc2);
}

def_EHelper(nemuor) {
  rtl_nemuor(s, ddest, dsrc1, dsrc2);
}

def_EHelper(nemuand) {
  rtl_nemuand(s, ddest, dsrc1, dsrc2);
}

def_EHelper(addi) {
  rtl_addi(s, ddest, dsrc1, id_src2->imm);
}

def_EHelper(slli) {
  rtl_shli(s, ddest, dsrc1, id_src2->imm);
}

def_EHelper(srai) {
  rtl_sari(s, ddest, dsrc1, id_src2->imm);
}

def_EHelper(srli) {
  rtl_shri(s, ddest, dsrc1, id_src2->imm);
}

def_EHelper(slti) {
  rtl_setrelopi(s, RELOP_LT, ddest, dsrc1, id_src2->imm);
}

def_EHelper(sltui) {
  rtl_setrelopi(s, RELOP_LTU, ddest, dsrc1, id_src2->imm);
}

def_EHelper(xori) {
  rtl_nemuxori(s, ddest, dsrc1, id_src2->imm);
}

def_EHelper(nemuori) {
  rtl_nemuori(s, ddest, dsrc1, id_src2->imm);
}

def_EHelper(nemuandi) {
  rtl_nemuandi(s, ddest, dsrc1, id_src2->imm);
}

def_EHelper(auipc) {
  rtl_li(s, ddest, id_src1->imm);
}

def_EHelper(lui) {
  rtl_li(s, ddest, id_src1->imm);
}

def_EHelper(addw) {
  rtl_addw(s, ddest, dsrc1, dsrc2);
}

def_EHelper(subw) {
  rtl_subw(s, ddest, dsrc1, dsrc2);
}

def_EHelper(sllw) {
  rtl_shlw(s, ddest, dsrc1, dsrc2);
}

def_EHelper(srlw) {
  rtl_shrw(s, ddest, dsrc1, dsrc2);
}

def_EHelper(sraw) {
  rtl_sarw(s, ddest, dsrc1, dsrc2);
}

def_EHelper(addiw) {
  rtl_addiw(s, ddest, dsrc1, id_src2->imm);
}

def_EHelper(slliw) {
  rtl_shliw(s, ddest, dsrc1, id_src2->imm);
}

def_EHelper(srliw) {
  rtl_shriw(s, ddest, dsrc1, id_src2->imm);
}

def_EHelper(sraiw) {
  rtl_sariw(s, ddest, dsrc1, id_src2->imm);
}
