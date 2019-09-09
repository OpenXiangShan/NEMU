#include "cpu/exec.h"

make_EHelper(mul) {
  rtl_mul_lo(&s0, &id_src->val, &id_src2->val);
  rtl_sr(id_dest->reg, &s0, 4);

  print_asm_template3(mul);
}

make_EHelper(mulh) {
  rtl_imul_hi(&s0, &id_src->val, &id_src2->val);
  rtl_sr(id_dest->reg, &s0, 4);

  print_asm_template3(mulh);
}

make_EHelper(mulhu) {
  rtl_mul_hi(&s0, &id_src->val, &id_src2->val);
  rtl_sr(id_dest->reg, &s0, 4);

  print_asm_template3(mulh);
}

make_EHelper(div) {
  rtl_idiv_q(&s0, &id_src->val, &id_src2->val);
  rtl_sr(id_dest->reg, &s0, 4);

  print_asm_template3(div);
}

make_EHelper(divu) {
  rtl_div_q(&s0, &id_src->val, &id_src2->val);
  rtl_sr(id_dest->reg, &s0, 4);

  print_asm_template3(divu);
}

make_EHelper(rem) {
  rtl_idiv_r(&s0, &id_src->val, &id_src2->val);
  rtl_sr(id_dest->reg, &s0, 4);

  print_asm_template3(rem);
}

make_EHelper(remu) {
  rtl_div_r(&s0, &id_src->val, &id_src2->val);
  rtl_sr(id_dest->reg, &s0, 4);

  print_asm_template3(remu);
}

make_EHelper(mulw) {
  rtl_mul_lo(&s0, &id_src->val, &id_src2->val);
  rtl_sext(&s0, &s0, 4);
  rtl_sr(id_dest->reg, &s0, 4);

  print_asm_template3(mulw);
}

make_EHelper(divw) {
  rtl_sext(&s0, &id_src->val, 4);
  rtl_sext(&s1, &id_src2->val, 4);
  rtl_idiv_q(&s0, &s0, &s1);
  rtl_sext(&s0, &s0, 4);
  rtl_sr(id_dest->reg, &s0, 4);

  print_asm_template3(divw);
}

make_EHelper(remw) {
  rtl_sext(&s0, &id_src->val, 4);
  rtl_sext(&s1, &id_src2->val, 4);
  rtl_idiv_r(&s0, &s0, &s1);
  rtl_sext(&s0, &s0, 4);
  rtl_sr(id_dest->reg, &s0, 4);

  print_asm_template3(remw);
}

make_EHelper(divuw) {
  rtl_andi(&s0, &id_src->val, 0xffffffffu);
  rtl_andi(&s1, &id_src2->val, 0xffffffffu);
  rtl_div_q(&s0, &s0, &s1);
  rtl_sext(&s0, &s0, 4);
  rtl_sr(id_dest->reg, &s0, 4);

  print_asm_template3(divuw);
}

make_EHelper(remuw) {
  rtl_andi(&s0, &id_src->val, 0xffffffffu);
  rtl_andi(&s1, &id_src2->val, 0xffffffffu);
  rtl_div_r(&s0, &s0, &s1);
  rtl_sext(&s0, &s0, 4);
  rtl_sr(id_dest->reg, &s0, 4);

  print_asm_template3(remuw);
}
