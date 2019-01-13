#include "cpu/exec.h"

make_EHelper(mfhi) {
  rtl_sr(id_dest->reg, &cpu.hi, 4);

  print_asm_template3(mfhi);
}

make_EHelper(mflo) {
  rtl_sr(id_dest->reg, &cpu.lo, 4);

  print_asm_template3(mflo);
}

make_EHelper(mul) {
  rtl_imul_lo(&t0, &id_src->val, &id_src2->val);
  rtl_sr(id_dest->reg, &t0, 4);

  print_asm_template3(mul);
}

make_EHelper(mult) {
  rtl_imul_lo(&cpu.lo, &id_src->val, &id_src2->val);
  rtl_imul_hi(&cpu.hi, &id_src->val, &id_src2->val);

  print_asm_template3(mult);
}

make_EHelper(div) {
  rtl_div_q(&cpu.lo, &id_src->val, &id_src2->val);
  rtl_div_r(&cpu.hi, &id_src->val, &id_src2->val);

  print_asm_template3(div);
}
