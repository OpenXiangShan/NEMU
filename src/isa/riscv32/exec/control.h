#include <monitor/difftest.h>

static inline make_EHelper(jal) {
  rtl_li(s, ddest, s->seq_pc);
  rtl_j(s, s->jmp_pc);

  print_asm_template2(jal);
}

static inline make_EHelper(jalr) {
  rtl_addi(s, s0, dsrc1, id_src2->imm);
#ifdef __ENGINE_interpreter__
  rtl_andi(s, s0, s0, ~0x1u);
#endif
  rtl_jr(s, s0);

  rtl_li(s, ddest, s->seq_pc);

#ifndef __DIFF_REF_NEMU__
  difftest_skip_dut(1, 2);
#endif

  print_asm_template3(jalr);
}

static const struct {
  int relop;
  char *name;
} branch_map [] = {
  [0] = { RELOP_EQ, "eq"},
  [1] = { RELOP_NE, "ne"},
  [4] = { RELOP_LT, "lt"},
  [5] = { RELOP_GE, "ge"},
  [6] = { RELOP_LTU, "ltu"},
  [7] = { RELOP_GEU, "geu"},
};

static inline make_EHelper(branch) {
  int type = s->isa.instr.b.funct3;
  assert(type != 2 && type != 3);
  rtl_jrelop(s, branch_map[type].relop, dsrc1, dsrc2, s->jmp_pc);

  print_asm("b%s %s,%s,0x%x", branch_map[type].name, id_src1->str, id_src2->str, s->jmp_pc);
}
