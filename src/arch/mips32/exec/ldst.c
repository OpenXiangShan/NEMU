#include "cpu/exec.h"

make_EHelper(load) {
  rtl_add(&t0, &id_src->val, &id_src2->val);
  rtl_lm(&t0, &t0, decinfo.width);
  rtl_sr(id_dest->reg, &t0, 4);

  print_asm_template3(load);
}

// load sign value
make_EHelper(loads) {
  rtl_add(&t0, &id_src->val, &id_src2->val);
  rtl_lm(&t0, &t0, decinfo.width);
  rtl_sext(&t0, &t0, decinfo.width);
  rtl_sr(id_dest->reg, &t0, 4);

  print_asm_template3(load);
}

make_EHelper(store) {
  rtl_add(&t0, &id_src->val, &id_src2->val);
  rtl_sm(&t0, &id_dest->val, decinfo.width);

  print_asm_template3(store);
}

make_EHelper(swl) {
  rtl_add(&t0, &id_src->val, &id_src2->val);

  // int reg_right_shift_amount [] = {24, 16, 8, 0};
  // (3 - n) << 3
  rtl_not(&t3, &t0);
  rtl_andi(&t3, &t3, 0x3);
  rtl_shli(&t3, &t3, 3);
  // prepare register data
  rtl_shr(&id_dest->val, &id_dest->val, &t3);

  // int mem_mask [] = {0xffffff00, 0xffff00, 0xff000000, 0x0};
  // (0x80000000 >> reg_right_shift_amount[n]) << 1
  rtl_li(&t2, 0x80000000u);
  rtl_sar(&t2, &t2, &t3);
  rtl_shli(&t2, &t2, 1);
  // load the aligned memory word
  rtl_andi(&t0, &t0, ~0x3u);
  rtl_lm(&t1, &t0, 4);
  // prepare memory data
  rtl_and(&t1, &t1, &t2);

  // merge the word
  rtl_or(&t1, &t1, &id_dest->val);

  // write back
  rtl_sm(&t0, &t1, 4);

  print_asm_template3(swl);
}

make_EHelper(swr) {
  rtl_add(&t0, &id_src->val, &id_src2->val);

  // int reg_left_shift_amount [] = {0, 8, 16, 24};
  // n << 3
  rtl_andi(&t3, &t0, 0x3);
  rtl_shli(&t3, &t3, 3);
  // prepare register data
  rtl_shl(&id_dest->val, &id_dest->val, &t3);

  // int mem_mask [] = {0x0, 0xff, 0xffff, 0xffffff};
  // ~(0xffffffff << reg_left_shift_amount[n])
  rtl_li(&t2, 0xffffffffu);
  rtl_shl(&t2, &t2, &t3);
  rtl_not(&t2, &t2);
  // load the aligned memory word
  rtl_andi(&t0, &t0, ~0x3u);
  rtl_lm(&t1, &t0, 4);
  // prepare memory data
  rtl_and(&t1, &t1, &t2);

  // merge the word
  rtl_or(&t1, &t1, &id_dest->val);

  // write back
  rtl_sm(&t0, &t1, 4);

  print_asm_template3(swr);
}

make_EHelper(lwl) {
  rtl_add(&t0, &id_src->val, &id_src2->val);

  // int mem_left_shift_amount [] = {24, 16, 8, 0};
  // (3 - n) << 3
  rtl_not(&t3, &t0);
  rtl_andi(&t3, &t3, 0x3);
  rtl_shli(&t3, &t3, 3);
  // load the aligned memory word
  rtl_andi(&t0, &t0, ~0x3u);
  rtl_lm(&t1, &t0, 4);
  // prepare memory data
  rtl_shl(&t1, &t1, &t3);

  // int reg_mask [] = {0xffffff, 0xffff, 0xff, 0x0};
  // ~(0xffffffff << mem_left_shift_amount[n])
  rtl_li(&t2, 0xffffffffu);
  rtl_shl(&t2, &t2, &t3);
  rtl_not(&t2, &t2);
  // prepare register data
  rtl_and(&id_dest->val, &id_dest->val, &t2);

  // merge the word
  rtl_or(&id_dest->val, &id_dest->val, &t1);

  // write back
  rtl_sr(id_dest->reg, &id_dest->val, 4);

  print_asm_template3(lwl);
}

make_EHelper(lwr) {
  rtl_add(&t0, &id_src->val, &id_src2->val);

  // int mem_right_shift_amount [] = {0, 8, 16, 24};
  // n << 3
  rtl_andi(&t3, &t0, 0x3);
  rtl_shli(&t3, &t3, 3);
  // load the aligned memory word
  rtl_andi(&t0, &t0, ~0x3u);
  rtl_lm(&t1, &t0, 4);
  // prepare memory data
  rtl_shr(&t1, &t1, &t3);


  // int reg_mask [] = {0x0, 0xff000000, 0xffff0000, 0xffffff00};
  // ~(0xffffffff >> mem_right_shift_amount[n])
  rtl_li(&t2, 0xffffffffu);
  rtl_shr(&t2, &t2, &t3);
  rtl_not(&t2, &t2);
  // prepare register data
  rtl_and(&id_dest->val, &id_dest->val, &t2);

  // merge the word
  rtl_or(&id_dest->val, &id_dest->val, &t1);

  // write back
  rtl_sr(id_dest->reg, &id_dest->val, 4);

  print_asm_template3(lwr);
}
