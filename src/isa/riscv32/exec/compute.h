def_EHelper(lui) {
  rtl_li(s, ddest, id_src1->imm);
  print_asm_template2(lui);
}
#ifndef __ICS_EXPORT

def_EHelper(add) {
  rtl_add(s, ddest, dsrc1, dsrc2);
  print_asm_template3(add);
}

def_EHelper(sub) {
  rtl_sub(s, ddest, dsrc1, dsrc2);
  print_asm_template3(sub);
}

def_EHelper(sll) {
  rtl_shl(s, ddest, dsrc1, dsrc2);
  print_asm_template3(sll);
}

def_EHelper(srl) {
  rtl_shr(s, ddest, dsrc1, dsrc2);
  print_asm_template3(srl);
}

def_EHelper(sra) {
  rtl_sar(s, ddest, dsrc1, dsrc2);
  print_asm_template3(sra);
}

def_EHelper(slt) {
  rtl_setrelop(s, RELOP_LT, ddest, dsrc1, dsrc2);
  print_asm_template3(slt);
}

def_EHelper(sltu) {
  rtl_setrelop(s, RELOP_LTU, ddest, dsrc1, dsrc2);
  print_asm_template3(sltu);
}

def_EHelper(xor) {
  rtl_xor(s, ddest, dsrc1, dsrc2);
  print_asm_template3(xor);
}

def_EHelper(or) {
  rtl_or(s, ddest, dsrc1, dsrc2);
  print_asm_template3(or);
}

def_EHelper(and) {
  rtl_and(s, ddest, dsrc1, dsrc2);
  print_asm_template3(and);
}

def_EHelper(addi) {
  rtl_addi(s, ddest, dsrc1, id_src2->imm);
  print_asm_template3(addi);
}

def_EHelper(slli) {
  rtl_shli(s, ddest, dsrc1, id_src2->imm);
  print_asm_template3(slli);
}

def_EHelper(srli) {
  rtl_shri(s, ddest, dsrc1, id_src2->imm);
  print_asm_template3(srli);
}

def_EHelper(srai) {
  rtl_sari(s, ddest, dsrc1, id_src2->imm);
  print_asm_template3(srai);
}


def_EHelper(slti) {
  rtl_setrelopi(s, RELOP_LT, ddest, dsrc1, id_src2->imm);
  print_asm_template3(slti);
}

def_EHelper(sltui) {
  rtl_setrelopi(s, RELOP_LTU, ddest, dsrc1, id_src2->imm);
  print_asm_template3(sltui);
}

def_EHelper(xori) {
  rtl_xori(s, ddest, dsrc1, id_src2->imm);
  print_asm_template3(xori);
}

def_EHelper(ori) {
  rtl_ori(s, ddest, dsrc1, id_src2->imm);
  print_asm_template3(ori);
}

def_EHelper(andi) {
  rtl_andi(s, ddest, dsrc1, id_src2->imm);
  print_asm_template3(andi);
}

def_EHelper(auipc) {
  rtl_li(s, ddest, id_src1->imm + s->pc);
  print_asm_template2(auipc);
}
#endif
