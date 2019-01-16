#include "cpu/exec.h"

make_EHelper(lui) {
  rtl_shli(&s0, &id_src2->val, 16);
  rtl_sr(id_dest->reg, &s0, 4);

  print_asm_template3(lui);
}

make_EHelper(add) {
  rtl_add(&s0, &id_src->val, &id_src2->val);
  rtl_sr(id_dest->reg, &s0, 4);

  print_asm_template3(add);
}

make_EHelper(sub) {
  rtl_sub(&s0, &id_src->val, &id_src2->val);
  rtl_sr(id_dest->reg, &s0, 4);

  print_asm_template3(sub);
}

make_EHelper(slt) {
  rtl_setrelop(RELOP_LT, &s0, &id_src->val, &id_src2->val);
  rtl_sr(id_dest->reg, &s0, 4);

  print_asm_template3(slt);
}

make_EHelper(sltu) {
  rtl_setrelop(RELOP_LTU, &s0, &id_src->val, &id_src2->val);
  rtl_sr(id_dest->reg, &s0, 4);

  print_asm_template3(sltu);
}

make_EHelper(and) {
  rtl_and(&s0, &id_src->val, &id_src2->val);
  rtl_sr(id_dest->reg, &s0, 4);

  print_asm_template3(and);
}

make_EHelper(or) {
  rtl_or(&s0, &id_src->val, &id_src2->val);
  rtl_sr(id_dest->reg, &s0, 4);

  print_asm_template3(or);
}

make_EHelper(xor) {
  rtl_xor(&s0, &id_src->val, &id_src2->val);
  rtl_sr(id_dest->reg, &s0, 4);

  print_asm_template3(xor);
}

make_EHelper(nor) {
  rtl_or(&s0, &id_src->val, &id_src2->val);
  rtl_not(&s0, &s0);
  rtl_sr(id_dest->reg, &s0, 4);

  print_asm_template3(nor);
}

make_EHelper(sll) {
  rtl_shl(&s0, &id_src2->val, &id_src->val);
  rtl_sr(id_dest->reg, &s0, 4);

  print_asm_template3(sll);
}

make_EHelper(srl) {
  rtl_shr(&s0, &id_src2->val, &id_src->val);
  rtl_sr(id_dest->reg, &s0, 4);

  print_asm_template3(srl);
}

make_EHelper(sra) {
  rtl_sar(&s0, &id_src2->val, &id_src->val);
  rtl_sr(id_dest->reg, &s0, 4);

  print_asm_template3(sra);
}

make_EHelper(movz) {
  rtl_mux(&s0, &id_src2->val, &id_dest->val, &id_src->val);
  rtl_sr(id_dest->reg, &s0, 4);

  print_asm_template3(movz);
}

make_EHelper(movn) {
  rtl_mux(&s0, &id_src2->val, &id_src->val, &id_dest->val);
  rtl_sr(id_dest->reg, &s0, 4);

  print_asm_template3(movn);
}
