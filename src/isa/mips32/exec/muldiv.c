#include "cpu/exec.h"

make_EHelper(mfhi) {
  rtl_sr(id_dest->reg, &cpu.hi, 4);

  print_asm_template3(mfhi);
}

make_EHelper(mflo) {
  rtl_sr(id_dest->reg, &cpu.lo, 4);

  print_asm_template3(mflo);
}

make_EHelper(mthi) {
  rtl_mv(&cpu.hi, &id_src->val);

  print_asm_template2(mthi);
}

make_EHelper(mtlo) {
  rtl_mv(&cpu.lo, &id_src->val);

  print_asm_template2(mtlo);
}

make_EHelper(mul) {
  rtl_imul_lo(&s0, &id_src->val, &id_src2->val);
  rtl_sr(id_dest->reg, &s0, 4);

  print_asm_template3(mul);
}

make_EHelper(mult) {
  rtl_imul_lo(&cpu.lo, &id_src->val, &id_src2->val);
  rtl_imul_hi(&cpu.hi, &id_src->val, &id_src2->val);

  print_asm_template3(mult);
}

make_EHelper(multu) {
  rtl_mul_lo(&cpu.lo, &id_src->val, &id_src2->val);
  rtl_mul_hi(&cpu.hi, &id_src->val, &id_src2->val);

  print_asm_template3(multu);
}

make_EHelper(div) {
  rtl_idiv_q(&cpu.lo, &id_src->val, &id_src2->val);
  rtl_idiv_r(&cpu.hi, &id_src->val, &id_src2->val);

  print_asm_template3(div);
}

make_EHelper(divu) {
  rtl_div_q(&cpu.lo, &id_src->val, &id_src2->val);
  rtl_div_r(&cpu.hi, &id_src->val, &id_src2->val);

  print_asm_template3(divu);
}
