#include "cpu/exec.h"

make_EHelper(jal) {
  rtl_li(&s0, decinfo.seq_pc);
  rtl_sr(id_dest->reg, &s0, 4);
  rtl_j(decinfo.jmp_pc);

  print_asm_template2(jal);
}

make_EHelper(jalr) {
  rtl_li(&s0, decinfo.seq_pc);
  rtl_sr(id_dest->reg, &s0, 4);

  rtl_add(&s0, &id_src->val, &id_src2->val);
  rtl_andi(&s0, &s0, ~1lu);
  rtl_jr(&s0);

  difftest_skip_dut(1, 2);

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

make_EHelper(branch) {
  int type = decinfo.isa.instr.funct3;
  assert(type != 2 && type != 3);
  rtl_jrelop(branch_map[type].relop, &id_src->val, &id_src2->val, decinfo.jmp_pc);

  print_asm("b%s %s,%s,%s", branch_map[type].name, id_src->str, id_src2->str, id_dest->str);
}

make_EHelper(beq) {
  rtl_jrelop(RELOP_EQ, &id_src->val, &id_src2->val, decinfo.jmp_pc);
  print_asm("beq %s,%s,%s", id_src->str, id_src2->str, id_dest->str);
}

make_EHelper(bne) {
  rtl_jrelop(RELOP_NE, &id_src->val, &id_src2->val, decinfo.jmp_pc);
  print_asm("bne %s,%s,%s", id_src->str, id_src2->str, id_dest->str);
}
