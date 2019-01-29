#include "cpu/exec.h"

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

make_EHelper(sll) {
  rtl_shl(&s0, &id_src->val, &id_src2->val);
  rtl_sr(id_dest->reg, &s0, 4);

  print_asm_template3(sll);
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

make_EHelper(xor) {
  rtl_xor(&s0, &id_src->val, &id_src2->val);
  rtl_sr(id_dest->reg, &s0, 4);

  print_asm_template3(xor);
}

make_EHelper(or) {
  rtl_or(&s0, &id_src->val, &id_src2->val);
  rtl_sr(id_dest->reg, &s0, 4);

  print_asm_template3(or);
}

make_EHelper(and) {
  rtl_and(&s0, &id_src->val, &id_src2->val);
  rtl_sr(id_dest->reg, &s0, 4);

  print_asm_template3(and);
}

make_EHelper(auipc) {
  rtl_add(&s0, &id_src->val, &cpu.pc);
  rtl_sr(id_dest->reg, &s0, 4);

  print_asm_template2(auipc);
}

make_EHelper(lui) {
  rtl_sr(id_dest->reg, &id_src->val, 4);

  print_asm_template2(lui);
}
