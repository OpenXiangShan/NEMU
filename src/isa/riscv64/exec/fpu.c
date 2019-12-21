#include "cpu/exec.h"

make_EHelper(fp_ld) {
  rtl_lm(&s0, &id_src->addr, decinfo.width);
  rtl_sfpr(id_dest->reg, &s0, 0);

  switch (decinfo.width) {
    case 8: print_asm_template2(fld); break;
    case 4: print_asm_template2(flw); break;
    default: assert(0);
  }
}

make_EHelper(fp_st) {
  rtl_sm(&id_src->addr, &id_dest->val, decinfo.width);

  switch (decinfo.width) {
    case 8: print_asm_template2(fsd); break;
    case 4: print_asm_template2(fsw); break;
    default: assert(0);
  }
}
