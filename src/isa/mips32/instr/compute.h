def_EHelper(lui) {
  rtl_li(s, ddest, id_src1->imm);
}

def_EHelper(add) {
  rtl_add(s, ddest, dsrc1, dsrc2);
}

def_EHelper(sub) {
  rtl_sub(s, ddest, dsrc1, dsrc2);
}

def_EHelper(slt) {
  rtl_setrelop(s, RELOP_LT, ddest, dsrc1, dsrc2);
}

def_EHelper(sltu) {
  rtl_setrelop(s, RELOP_LTU, ddest, dsrc1, dsrc2);
}

def_EHelper(and) {
  rtl_and(s, ddest, dsrc1, dsrc2);
}

def_EHelper(or) {
  rtl_or(s, ddest, dsrc1, dsrc2);
}

def_EHelper(xor) {
  rtl_xor(s, ddest, dsrc1, dsrc2);
}

def_EHelper(nor) {
  rtl_or(s, ddest, dsrc1, dsrc2);
  rtl_not(s, ddest, ddest);
}

def_EHelper(sll) {
  rtl_shl(s, ddest, dsrc2, dsrc1);
}

def_EHelper(srl) {
  rtl_shr(s, ddest, dsrc2, dsrc1);
}

def_EHelper(sra) {
  rtl_sar(s, ddest, dsrc2, dsrc1);
}

def_EHelper(addi) {
  rtl_addi(s, ddest, dsrc1, id_src2->imm);
}

def_EHelper(slti) {
  rtl_setrelopi(s, RELOP_LT, ddest, dsrc1, id_src2->imm);
}

def_EHelper(sltui) {
  rtl_setrelopi(s, RELOP_LTU, ddest, dsrc1, id_src2->imm);
}

def_EHelper(andi) {
  rtl_andi(s, ddest, dsrc1, id_src2->imm);
}

def_EHelper(ori) {
  rtl_ori(s, ddest, dsrc1, id_src2->imm);
}

def_EHelper(xori) {
  rtl_xori(s, ddest, dsrc1, id_src2->imm);
}

def_EHelper(slli) {
  rtl_shli(s, ddest, dsrc2, id_src1->imm);
}

def_EHelper(srli) {
  rtl_shri(s, ddest, dsrc2, id_src1->imm);
}

def_EHelper(srai) {
  rtl_sari(s, ddest, dsrc2, id_src1->imm);
}

def_EHelper(movz) {
  rtl_mux(s, ddest, dsrc2, ddest, dsrc1);
}

def_EHelper(movn) {
  rtl_mux(s, ddest, dsrc2, dsrc1, ddest);
}

def_EHelper(clz) {
#ifndef CONFIG_ENGINE_INTERPRETER
  panic("not support in engines other than interpreter");
#endif

  int bit = 32;
  int i;
  for (i = 31; i >= 0; i ++) {
    if (*dsrc1 & (1u << i)) {
      bit = 31 - i;
      break;
    }
  }
  *ddest = bit;
}
