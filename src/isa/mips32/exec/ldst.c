#include "cpu/exec.h"

make_EHelper(ld) {
  rtl_lm(&s0, &id_src->addr, decinfo.width);
  rtl_sr(id_dest->reg, &s0, 4);

  switch (decinfo.width) {
    case 4: print_asm_template2(lw); break;
    case 2: print_asm_template2(lhu); break;
    case 1: print_asm_template2(lbu); break;
    default: assert(0);
  }
}

// load sign value
make_EHelper(lds) {
  rtl_lm(&s0, &id_src->addr, decinfo.width);
  rtl_sext(&s0, &s0, decinfo.width);
  rtl_sr(id_dest->reg, &s0, 4);

  switch (decinfo.width) {
    case 2: print_asm_template2(lh); break;
    case 1: print_asm_template2(lb); break;
    default: assert(0);
  }
}

make_EHelper(st) {
  rtl_sm(&id_src->addr, &id_dest->val, decinfo.width);

  switch (decinfo.width) {
    case 4: print_asm_template2(sw); break;
    case 2: print_asm_template2(sh); break;
    case 1: print_asm_template2(sb); break;
    default: assert(0);
  }
}

make_EHelper(swl) {
  // int reg_right_shift_amount [] = {24, 16, 8, 0};
  // (3 - n) << 3
  rtl_not(&s0, &id_src->addr);
  rtl_andi(&s0, &s0, 0x3);
  rtl_shli(&s0, &s0, 3);
  // prepare register data
  rtl_shr(&id_dest->val, &id_dest->val, &s0);

  // int mem_mask [] = {0xffffff00, 0xffff00, 0xff000000, 0x0};
  // (0x80000000 >> reg_right_shift_amount[n]) << 1
  rtl_li(&s1, 0x80000000u);
  rtl_sar(&s1, &s1, &s0);
  rtl_shli(&s1, &s1, 1);
  // load the aligned memory word
  rtl_andi(&id_src->addr, &id_src->addr, ~0x3u);
  rtl_lm(&s0, &id_src->addr, 4);
  // prepare memory data
  rtl_and(&s0, &s0, &s1);

  // merge the word
  rtl_or(&s0, &s0, &id_dest->val);

  // write back
  rtl_sm(&id_src->addr, &s0, 4);

  print_asm_template2(swl);
}

make_EHelper(swr) {
  // int reg_left_shift_amount [] = {0, 8, 16, 24};
  // n << 3
  rtl_andi(&s0, &id_src->addr, 0x3);
  rtl_shli(&s0, &s0, 3);
  // prepare register data
  rtl_shl(&id_dest->val, &id_dest->val, &s0);

  // int mem_mask [] = {0x0, 0xff, 0xffff, 0xffffff};
  // ~(0xffffffff << reg_left_shift_amount[n])
  rtl_li(&s1, 0xffffffffu);
  rtl_shl(&s1, &s1, &s0);
  rtl_not(&s1, &s1);
  // load the aligned memory word
  rtl_andi(&id_src->addr, &id_src->addr, ~0x3u);
  rtl_lm(&s0, &id_src->addr, 4);
  // prepare memory data
  rtl_and(&s0, &s0, &s1);

  // merge the word
  rtl_or(&s0, &s0, &id_dest->val);

  // write back
  rtl_sm(&id_src->addr, &s0, 4);

  print_asm_template2(swr);
}

make_EHelper(lwl) {
  // int mem_left_shift_amount [] = {24, 16, 8, 0};
  // (3 - n) << 3
  rtl_not(&s0, &id_src->addr);
  rtl_andi(&s0, &s0, 0x3);
  rtl_shli(&s0, &s0, 3);

  // int reg_mask [] = {0xffffff, 0xffff, 0xff, 0x0};
  // ~(0xffffffff << mem_left_shift_amount[n])
  rtl_li(&s1, 0xffffffffu);
  rtl_shl(&s1, &s1, &s0);
  rtl_not(&s1, &s1);
  // prepare register data
  rtl_and(&id_dest->val, &id_dest->val, &s1);

  // load the aligned memory word
  rtl_andi(&id_src->addr, &id_src->addr, ~0x3u);
  rtl_lm(&s1, &id_src->addr, 4);
  // prepare memory data
  rtl_shl(&s1, &s1, &s0);

  // merge the word
  rtl_or(&id_dest->val, &id_dest->val, &s1);

  // write back
  rtl_sr(id_dest->reg, &id_dest->val, 4);

  print_asm_template2(lwl);
}

make_EHelper(lwr) {
  // int mem_right_shift_amount [] = {0, 8, 16, 24};
  // n << 3
  rtl_andi(&s0, &id_src->addr, 0x3);
  rtl_shli(&s0, &s0, 3);

  // int reg_mask [] = {0x0, 0xff000000, 0xffff0000, 0xffffff00};
  // ~(0xffffffff >> mem_right_shift_amount[n])
  rtl_li(&s1, 0xffffffffu);
  rtl_shr(&s1, &s1, &s0);
  rtl_not(&s1, &s1);
  // prepare register data
  rtl_and(&id_dest->val, &id_dest->val, &s1);

  // load the aligned memory word
  rtl_andi(&id_src->addr, &id_src->addr, ~0x3u);
  rtl_lm(&s1, &id_src->addr, 4);
  // prepare memory data
  rtl_shr(&s1, &s1, &s0);

  // merge the word
  rtl_or(&id_dest->val, &id_dest->val, &s1);

  // write back
  rtl_sr(id_dest->reg, &id_dest->val, 4);

  print_asm_template2(lwr);
}
