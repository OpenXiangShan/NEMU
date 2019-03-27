#include "cpu/exec.h"

static inline void difftest_skip_delay_slot(void) {
#if defined(DIFF_TEST)
  if ((cpu.pc & 0xfff) == 0xffc) {
    difftest_skip_ref();
  }
#endif
}

make_EHelper(j) {
  difftest_skip_delay_slot();

  rtl_j(decinfo.jmp_pc);

  print_asm_template1(j);
}

make_EHelper(jal) {
  difftest_skip_delay_slot();

  rtl_addi(&s0, &cpu.pc, 8);
  rtl_sr(31, &s0, 4);
  rtl_j(decinfo.jmp_pc);

  print_asm_template1(jal);
}

make_EHelper(jr) {
  difftest_skip_delay_slot();

  rtl_jr(&id_src->val);

  print_asm_template2(jr);
}

make_EHelper(jalr) {
  difftest_skip_delay_slot();

  rtl_addi(&s0, &cpu.pc, 8);
  rtl_sr(id_dest->reg, &s0, 4);
  rtl_jr(&id_src->val);

  print_asm_template2(jalr);
}

make_EHelper(bne) {
  difftest_skip_delay_slot();

  rtl_jrelop(RELOP_NE, &id_src->val, &id_src2->val, decinfo.jmp_pc);

  print_asm_template3(bne);
}

make_EHelper(beq) {
  difftest_skip_delay_slot();

  rtl_jrelop(RELOP_EQ, &id_src->val, &id_src2->val, decinfo.jmp_pc);

  print_asm_template3(beq);
}

make_EHelper(blez) {
  difftest_skip_delay_slot();

  rtl_jrelop(RELOP_LE, &id_src->val, &id_src2->val, decinfo.jmp_pc);

  print_asm_template3(blez);
}

make_EHelper(bltz) {
  difftest_skip_delay_slot();

  rtl_li(&s0, 0);
  rtl_jrelop(RELOP_LT, &id_src->val, &s0, decinfo.jmp_pc);

  print_asm_template3(bltz);
}

make_EHelper(bgtz) {
  difftest_skip_delay_slot();

  rtl_jrelop(RELOP_GT, &id_src->val, &id_src2->val, decinfo.jmp_pc);

  print_asm_template3(blez);
}

make_EHelper(bgez) {
  difftest_skip_delay_slot();

  rtl_li(&s0, 0);
  rtl_jrelop(RELOP_GE, &id_src->val, &s0, decinfo.jmp_pc);

  print_asm_template3(bltz);
}
