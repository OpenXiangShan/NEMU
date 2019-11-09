#include "cpu/exec.h"

make_EHelper(amoswap) {
  rtl_lm(&s0, &id_src->val, decinfo.width);
  rtl_sext(&s0, &s0, decinfo.width);
  rtl_sr(id_dest->reg, &s0, 0);

  // swap
  rtl_sm(&id_src->val, &id_src2->val, decinfo.width);

  print_asm_template3(amoswap);
}

make_EHelper(amoor) {
  rtl_lm(&s0, &id_src->val, decinfo.width);
  rtl_sext(&s0, &s0, decinfo.width);
  rtl_sr(id_dest->reg, &s0, 0);

  // or
  rtl_or(&s0, &s0, &id_src2->val);
  rtl_sm(&id_src->val, &s0, decinfo.width);

  print_asm_template3(amoor);
}
