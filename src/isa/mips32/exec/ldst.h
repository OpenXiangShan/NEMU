#include "../local-include/intr.h"

static inline make_EHelper(ld) {
  rtl_lm(s, s0, dsrc1, id_src2->imm, s->width);
  check_mem_ex();
  rtl_sr(s, id_dest->reg, s0, 4);

  print_Dop(id_src1->str, OP_STR_SIZE, "%d(%s)", id_src2->imm, reg_name(id_src1->reg, 4));
  switch (s->width) {
    case 4: print_asm_template2(lw); break;
    case 2: print_asm_template2(lhu); break;
    case 1: print_asm_template2(lbu); break;
    default: assert(0);
  }
}

// load sign value
static inline make_EHelper(lds) {
  rtl_lm(s, s0, dsrc1, id_src2->imm, s->width);
  check_mem_ex();
  rtl_sext(s, ddest, s0, s->width);

  print_Dop(id_src1->str, OP_STR_SIZE, "%d(%s)", id_src2->imm, reg_name(id_src1->reg, 4));
  switch (s->width) {
    case 2: print_asm_template2(lh); break;
    case 1: print_asm_template2(lb); break;
    default: assert(0);
  }
}

static inline make_EHelper(st) {
  rtl_sm(s, dsrc1, id_src2->imm, ddest, s->width);
  check_mem_ex();

  print_Dop(id_src1->str, OP_STR_SIZE, "%d(%s)", id_src2->imm, reg_name(id_src1->reg, 4));
  switch (s->width) {
    case 4: print_asm_template2(sw); break;
    case 2: print_asm_template2(sh); break;
    case 1: print_asm_template2(sb); break;
    default: assert(0);
  }
}

// we do not use pseudo-rtl here, so we can use t0/t1 safely
static inline make_EHelper(swl) {
  // int reg_right_shift_amount [] = {24, 16, 8, 0};
  // (3 - n) << 3
  rtl_addi(s, t0, dsrc1, id_src2->imm); // t0 = addr
  rtl_not(s, s0, t0);
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
  rtl_andi(s, t0, t0, ~0x3u);
  rtl_lm(s, s0, t0, 0, 4);
  check_mem_ex();
  // prepare memory data
  rtl_and(s, s0, s0, s1);

  // merge the word
  rtl_or(s, s0, s0, ddest);

  // write back
  rtl_sm(s, t0, 0, s0, 4);
  check_mem_ex();

  print_Dop(id_src1->str, OP_STR_SIZE, "%d(%s)", id_src2->imm, reg_name(id_src1->reg, 4));
  print_asm_template2(swl);
}

static inline make_EHelper(swr) {
  // int reg_left_shift_amount [] = {0, 8, 16, 24};
  // n << 3
  rtl_addi(s, t0, dsrc1, id_src2->imm); // t0 = addr
  rtl_andi(s, s0, t0, 0x3);
  rtl_shli(s, s0, s0, 3);
  // prepare register data
  rtl_shl(s, ddest, ddest, s0);

  // int mem_mask [] = {0x0, 0xff, 0xffff, 0xffffff};
  // ~(0xffffffff << reg_left_shift_amount[n])
  rtl_li(s, s1, 0xffffffffu);
  rtl_shl(s, s1, s1, s0);
  rtl_not(s, s1, s1);
  // load the aligned memory word
  rtl_andi(s, t0, t0, ~0x3u);
  rtl_lm(s, s0, t0, 0, 4);
  check_mem_ex();
  // prepare memory data
  rtl_and(s, s0, s0, s1);

  // merge the word
  rtl_or(s, s0, s0, ddest);

  // write back
  rtl_sm(s, t0, 0, s0, 4);
  check_mem_ex();

  print_Dop(id_src1->str, OP_STR_SIZE, "%d(%s)", id_src2->imm, reg_name(id_src1->reg, 4));
  print_asm_template2(swr);
}

static inline make_EHelper(lwl) {
  // int mem_left_shift_amount [] = {24, 16, 8, 0};
  // (3 - n) << 3
  rtl_addi(s, t0, dsrc1, id_src2->imm); // t0 = addr
  rtl_not(s, s0, t0);
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
  rtl_andi(s, t0, t0, ~0x3u);
  rtl_lm(s, s1, t0, 0, 4);
  check_mem_ex();
  // prepare memory data
  rtl_shl(s, s1, s1, s0);

  // merge the word
  rtl_or(s, ddest, ddest, s1);

  // write back
  rtl_sr(s, id_dest->reg, ddest, 4);

  print_Dop(id_src1->str, OP_STR_SIZE, "%d(%s)", id_src2->imm, reg_name(id_src1->reg, 4));
  print_asm_template2(lwl);
}

static inline make_EHelper(lwr) {
  // int mem_right_shift_amount [] = {0, 8, 16, 24};
  // n << 3
  rtl_addi(s, t0, dsrc1, id_src2->imm); // t0 = addr
  rtl_andi(s, s0, t0, 0x3);
  rtl_shli(s, s0, s0, 3);

  // int reg_mask [] = {0x0, 0xff000000, 0xffff0000, 0xffffff00};
  // ~(0xffffffff >> mem_right_shift_amount[n])
  rtl_li(s, s1, 0xffffffffu);
  rtl_shr(s, s1, s1, s0);
  rtl_not(s, s1, s1);
  // prepare register data
  rtl_and(s, ddest, ddest, s1);

  // load the aligned memory word
  rtl_andi(s, t0, t0, ~0x3u);
  rtl_lm(s, s1, t0, 0, 4);
  check_mem_ex();
  // prepare memory data
  rtl_shr(s, s1, s1, s0);

  // merge the word
  rtl_or(s, ddest, ddest, s1);

  // write back
  rtl_sr(s, id_dest->reg, ddest, 4);

  print_Dop(id_src1->str, OP_STR_SIZE, "%d(%s)", id_src2->imm, reg_name(id_src1->reg, 4));
  print_asm_template2(lwr);
}
