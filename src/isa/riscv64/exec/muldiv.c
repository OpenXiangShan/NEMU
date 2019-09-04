#include "cpu/exec.h"

make_EHelper(mul) {
  rtl_imul_lo(&s0, &id_src->val, &id_src2->val);
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
  rtl_imul_lo(&s0, &id_src->val, &id_src2->val);
  rtl_li(&s1, 32);
  rtl_shl(&s0, &s0, &s1);
  rtl_sar64(&s0, &s0, &s1);
  rtl_sr(id_dest->reg, &s0, 4);

  print_asm_template3(mulw);
}

make_EHelper(divw) {
  rtl_idiv_q64(&s0, &id_src->val, &id_src2->val);
  rtl_li(&s1, 32);
  rtl_shl(&s0, &s0, &s1);
  rtl_sar64(&s0, &s0, &s1);
  rtl_sr(id_dest->reg, &s0, 4);

  print_asm_template3(div);
}

make_EHelper(remw) {
  rtl_idiv_r64(&s0, &id_src->val, &id_src2->val);
  rtl_li(&s1, 32);
  rtl_shl(&s0, &s0, &s1);
  rtl_sar64(&s0, &s0, &s1);
  rtl_sr(id_dest->reg, &s0, 4);

  print_asm_template3(remw);
}

make_EHelper(divuw) {
  rtl_div_q64(&s0, &id_src->val, &id_src2->val);
  rtl_li(&s1, 32);
  rtl_shl(&s0, &s0, &s1);
  rtl_sar64(&s0, &s0, &s1);
  rtl_sr(id_dest->reg, &s0, 4);

  print_asm_template3(divuw);
}

make_EHelper(remuw) {
  rtl_idiv_r64(&s0, &id_src->val, &id_src2->val);
  rtl_li(&s1, 32);
  rtl_shl(&s0, &s0, &s1);
  rtl_sar64(&s0, &s0, &s1);
  rtl_sr(id_dest->reg, &s0, 4);

  print_asm_template3(remuw);
}