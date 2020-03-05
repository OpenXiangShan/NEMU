static inline make_EHelper(ld) {
  rtl_lm(s, s0, &id_src1->addr, s->width);
  rtl_sr(s, id_dest->reg, s0, 4);

  switch (s->width) {
    case 4: print_asm_template2(lw); break;
    case 2: print_asm_template2(lhu); break;
    case 1: print_asm_template2(lbu); break;
    default: assert(0);
  }
}

// load sign value
static inline make_EHelper(lds) {
  rtl_lm(s, s0, &id_src1->addr, s->width);
  rtl_sext(s, s0, s0, s->width);
  rtl_sr(s, id_dest->reg, s0, 4);

  switch (s->width) {
    case 2: print_asm_template2(lh); break;
    case 1: print_asm_template2(lb); break;
    default: assert(0);
  }
}

static inline make_EHelper(st) {
  rtl_sm(s, &id_src1->addr, ddest, s->width);

  switch (s->width) {
    case 4: print_asm_template2(sw); break;
    case 2: print_asm_template2(sh); break;
    case 1: print_asm_template2(sb); break;
    default: assert(0);
  }
}

static inline make_EHelper(swl) {
  // int reg_right_shift_amount [] = {24, 16, 8, 0};
  // (3 - n) << 3
  rtl_not(s, s0, &id_src1->addr);
  rtl_andi(s, s0, s0, 0x3);
  rtl_shli(s, s0, s0, 3);
  // prepare register data
  rtl_shr(s, ddest, ddest, s0);

  // int mem_mask [] = {0xffffff00, 0xffff00, 0xff000000, 0x0};
  // (0x80000000 >> reg_right_shift_amount[n]) << 1
  rtl_li(s, s1, 0x80000000u);
  rtl_sar(s, s1, s1, s0);
  rtl_shli(s, s1, s1, 1);
  // load the aligned memory word
  rtl_andi(s, &id_src1->addr, &id_src1->addr, ~0x3u);
  rtl_lm(s, s0, &id_src1->addr, 4);
  // prepare memory data
  rtl_and(s, s0, s0, s1);

  // merge the word
  rtl_or(s, s0, s0, ddest);

  // write back
  rtl_sm(s, &id_src1->addr, s0, 4);

  print_asm_template2(swl);
}

static inline make_EHelper(swr) {
  // int reg_left_shift_amount [] = {0, 8, 16, 24};
  // n << 3
  rtl_andi(s, s0, &id_src1->addr, 0x3);
  rtl_shli(s, s0, s0, 3);
  // prepare register data
  rtl_shl(s, ddest, ddest, s0);

  // int mem_mask [] = {0x0, 0xff, 0xffff, 0xffffff};
  // ~(0xffffffff << reg_left_shift_amount[n])
  rtl_li(s, s1, 0xffffffffu);
  rtl_shl(s, s1, s1, s0);
  rtl_not(s, s1, s1);
  // load the aligned memory word
  rtl_andi(s, &id_src1->addr, &id_src1->addr, ~0x3u);
  rtl_lm(s, s0, &id_src1->addr, 4);
  // prepare memory data
  rtl_and(s, s0, s0, s1);

  // merge the word
  rtl_or(s, s0, s0, ddest);

  // write back
  rtl_sm(s, &id_src1->addr, s0, 4);

  print_asm_template2(swr);
}

static inline make_EHelper(lwl) {
  // int mem_left_shift_amount [] = {24, 16, 8, 0};
  // (3 - n) << 3
  rtl_not(s, s0, &id_src1->addr);
  rtl_andi(s, s0, s0, 0x3);
  rtl_shli(s, s0, s0, 3);

  // int reg_mask [] = {0xffffff, 0xffff, 0xff, 0x0};
  // ~(0xffffffff << mem_left_shift_amount[n])
  rtl_li(s, s1, 0xffffffffu);
  rtl_shl(s, s1, s1, s0);
  rtl_not(s, s1, s1);
  // prepare register data
  rtl_and(s, ddest, ddest, s1);

  // load the aligned memory word
  rtl_andi(s, &id_src1->addr, &id_src1->addr, ~0x3u);
  rtl_lm(s, s1, &id_src1->addr, 4);
  // prepare memory data
  rtl_shl(s, s1, s1, s0);

  // merge the word
  rtl_or(s, ddest, ddest, s1);

  // write back
  rtl_sr(s, id_dest->reg, ddest, 4);

  print_asm_template2(lwl);
}

static inline make_EHelper(lwr) {
  // int mem_right_shift_amount [] = {0, 8, 16, 24};
  // n << 3
  rtl_andi(s, s0, &id_src1->addr, 0x3);
  rtl_shli(s, s0, s0, 3);

  // int reg_mask [] = {0x0, 0xff000000, 0xffff0000, 0xffffff00};
  // ~(0xffffffff >> mem_right_shift_amount[n])
  rtl_li(s, s1, 0xffffffffu);
  rtl_shr(s, s1, s1, s0);
  rtl_not(s, s1, s1);
  // prepare register data
  rtl_and(s, ddest, ddest, s1);

  // load the aligned memory word
  rtl_andi(s, &id_src1->addr, &id_src1->addr, ~0x3u);
  rtl_lm(s, s1, &id_src1->addr, 4);
  // prepare memory data
  rtl_shr(s, s1, s1, s0);

  // merge the word
  rtl_or(s, ddest, ddest, s1);

  // write back
  rtl_sr(s, id_dest->reg, ddest, 4);

  print_asm_template2(lwr);
}
