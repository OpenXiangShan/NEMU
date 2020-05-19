#include <monitor/difftest.h>

static inline make_EHelper(jal) {
  rtl_li(s, ddest, s->seq_pc);
  rtl_j(s, s->jmp_pc);

  print_asm_template2(jal);
}

static inline make_EHelper(jalr) {
  rtl_addi(s, s0, dsrc1, id_src2->imm);
#ifdef __ENGINE_interpreter__
  rtl_andi(s, s0, s0, ~0x1lu);
#endif
  rtl_jr(s, s0);

  rtl_li(s, ddest, s->seq_pc);

#ifndef __DIFF_REF_NEMU__
  difftest_skip_dut(1, 2);
#endif

  print_asm_template3(jalr);
}

static inline make_EHelper(beq) {
  rtl_jrelop(s, RELOP_EQ, dsrc1, dsrc2, s->jmp_pc);
  print_asm_template3(beq);
}

static inline make_EHelper(bne) {
  rtl_jrelop(s, RELOP_NE, dsrc1, dsrc2, s->jmp_pc);
  print_asm_template3(bne);
}

static inline make_EHelper(blt) {
  rtl_jrelop(s, RELOP_LT, dsrc1, dsrc2, s->jmp_pc);
  print_asm_template3(blt);
}

static inline make_EHelper(bge) {
  rtl_jrelop(s, RELOP_GE, dsrc1, dsrc2, s->jmp_pc);
  print_asm_template3(bge);
}

static inline make_EHelper(bltu) {
  rtl_jrelop(s, RELOP_LTU, dsrc1, dsrc2, s->jmp_pc);
  print_asm_template3(bltu);
}

static inline make_EHelper(bgeu) {
  rtl_jrelop(s, RELOP_GEU, dsrc1, dsrc2, s->jmp_pc);
  print_asm_template3(bgeu);
}
