#include "cpu/exec.h"

make_EHelper(lui) {
  rtl_shli(&t0, &id_src2->val, 16);
  rtl_sr(id_dest->reg, &t0, 4);

  print_asm_template3(lui);
}

make_EHelper(add) {
  rtl_add(&t0, &id_src->val, &id_src2->val);
  rtl_sr(id_dest->reg, &t0, 4);

  print_asm_template3(add);
}

make_EHelper(sub) {
  rtl_sub(&t0, &id_src->val, &id_src2->val);
  rtl_sr(id_dest->reg, &t0, 4);

  print_asm_template3(sub);
}

make_EHelper(slt) {
  rtl_setrelop(RELOP_LT, &t0, &id_src->val, &id_src2->val);
  rtl_sr(id_dest->reg, &t0, 4);

  print_asm_template3(slt);
}

make_EHelper(sltu) {
  rtl_setrelop(RELOP_LTU, &t0, &id_src->val, &id_src2->val);
  rtl_sr(id_dest->reg, &t0, 4);

  print_asm_template3(sltu);
}

make_EHelper(or) {
  rtl_or(&t0, &id_src->val, &id_src2->val);
  rtl_sr(id_dest->reg, &t0, 4);

  print_asm_template3(or);
}

make_EHelper(xor) {
  rtl_xor(&t0, &id_src->val, &id_src2->val);
  rtl_sr(id_dest->reg, &t0, 4);

  print_asm_template3(xor);
}

make_EHelper(sll) {
  rtl_shl(&t0, &id_src2->val, &id_src->val);
  rtl_sr(id_dest->reg, &t0, 4);

  print_asm_template3(sll);
}

make_EHelper(srl) {
  rtl_shr(&t0, &id_src2->val, &id_src->val);
  rtl_sr(id_dest->reg, &t0, 4);

  print_asm_template3(srl);
}

make_EHelper(sra) {
  rtl_sar(&t0, &id_src2->val, &id_src->val);
  rtl_sr(id_dest->reg, &t0, 4);

  print_asm_template3(sra);
}

make_EHelper(movn) {
  rtl_mux(&t1, &id_src2->val, &id_src->val, &id_dest->val);
  rtl_sr(id_dest->reg, &t1, 4);

  print_asm_template3(movn);
}
