#include "cpu/exec.h"

make_EHelper(lr) {
  rtl_lm(&s0, &id_src->val, decinfo.width);
  rtl_sext(&s0, &s0, decinfo.width);
  rtl_sr(id_dest->reg, &s0, 0);

  cpu.lr_addr = id_src->val;
  // QEMU do not record the LR bit, is it safe?

  print_asm_template3(lr);
}

make_EHelper(sc) {
  // should check overlapping instead of equality
  if (cpu.lr_addr == id_src->val) {
    rtl_sm(&id_src->val, &id_src2->val, decinfo.width);
    rtl_li(&s0, 0);
  } else {
    rtl_li(&s0, 1);
  }
  rtl_sr(id_dest->reg, &s0, 0);

  print_asm_template3(sc);
}

static void inline amo_load() {
  rtl_lm(&s0, &id_src->val, decinfo.width);
  rtl_sext(&s0, &s0, decinfo.width);
}

static void inline amo_update() {
  rtl_sm(&id_src->val, &s1, decinfo.width);
  rtl_sr(id_dest->reg, &s0, 0);
}

make_EHelper(amoswap) {
  amo_load();
  rtl_mv(&s1, &id_src2->val); // swap
  amo_update();
  print_asm_template3(amoswap);
}

make_EHelper(amoadd) {
  amo_load();
  rtl_add(&s1, &s0, &id_src2->val);
  amo_update();
  print_asm_template3(amoor);
}

make_EHelper(amoor) {
  amo_load();
  rtl_or(&s1, &s0, &id_src2->val);
  amo_update();
  print_asm_template3(amoor);
}

make_EHelper(amoand) {
  amo_load();
  rtl_and(&s1, &s0, &id_src2->val);
  amo_update();
  print_asm_template3(amoand);
}

make_EHelper(amomaxu) {
  amo_load();
  s1 = (s0 > id_src2->val ? s0 : id_src2->val);
  amo_update();
  print_asm_template3(amomaxu);
}

make_EHelper(amoxor) {
  amo_load();
  rtl_xor(&s1, &s0, &id_src2->val);
  amo_update();
  print_asm_template3(amoxor);
}
