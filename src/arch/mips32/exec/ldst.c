#include "cpu/exec.h"

make_EHelper(load) {
  rtl_add(&t0, &id_src->val, &id_src2->val);
  rtl_lm(&t0, &t0, 4);
  rtl_sr(id_dest->reg, &t0, 4);

  print_asm_template3(load);
}

make_EHelper(store) {
  rtl_add(&t0, &id_src->val, &id_src2->val);
  rtl_sm(&t0, &id_dest->val, 4);

  print_asm_template3(store);
}
