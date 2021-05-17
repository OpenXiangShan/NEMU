def_EHelper(lui) {
  rtl_li(s, ddest, id_src1->imm);
}
#ifndef __ICS_EXPORT

def_EHelper(add) {
  rtl_add(s, ddest, dsrc1, dsrc2);
}

def_EHelper(sub) {
  rtl_sub(s, ddest, dsrc1, dsrc2);
}

def_EHelper(sll) {
  rtl_shl(s, ddest, dsrc1, dsrc2);
}

def_EHelper(srl) {
  rtl_shr(s, ddest, dsrc1, dsrc2);
}

def_EHelper(sra) {
  rtl_sar(s, ddest, dsrc1, dsrc2);
}

def_EHelper(slt) {
  rtl_setrelop(s, RELOP_LT, ddest, dsrc1, dsrc2);
}

def_EHelper(sltu) {
  rtl_setrelop(s, RELOP_LTU, ddest, dsrc1, dsrc2);
}

def_EHelper(xor) {
  rtl_xor(s, ddest, dsrc1, dsrc2);
}

def_EHelper(or) {
  rtl_or(s, ddest, dsrc1, dsrc2);
}

def_EHelper(and) {
  rtl_and(s, ddest, dsrc1, dsrc2);
}

def_EHelper(addi) {
  rtl_addi(s, ddest, dsrc1, id_src2->imm);
}

def_EHelper(slli) {
  rtl_shli(s, ddest, dsrc1, id_src2->imm);
}

def_EHelper(srli) {
  rtl_shri(s, ddest, dsrc1, id_src2->imm);
}

def_EHelper(srai) {
  rtl_sari(s, ddest, dsrc1, id_src2->imm);
}


def_EHelper(slti) {
  rtl_setrelopi(s, RELOP_LT, ddest, dsrc1, id_src2->imm);
}

def_EHelper(sltui) {
  rtl_setrelopi(s, RELOP_LTU, ddest, dsrc1, id_src2->imm);
}

def_EHelper(xori) {
  rtl_xori(s, ddest, dsrc1, id_src2->imm);
}

def_EHelper(ori) {
  rtl_ori(s, ddest, dsrc1, id_src2->imm);
}

def_EHelper(andi) {
  rtl_andi(s, ddest, dsrc1, id_src2->imm);
}

def_EHelper(auipc) {
  rtl_li(s, ddest, id_src1->imm);
}
#endif
