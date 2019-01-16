#include "cpu/exec.h"

make_EHelper(j) {
  rtl_j(decinfo.jmp_pc);

  print_asm_template1(j);
}

make_EHelper(jal) {
  rtl_addi(&s0, &cpu.pc, 8);
  rtl_sr(31, &s0, 4);
  rtl_j(decinfo.jmp_pc);

  print_asm_template1(jal);
}

make_EHelper(jr) {
  rtl_jr(&id_src->val);

  print_asm_template2(jr);
}

make_EHelper(jalr) {
  rtl_addi(&s0, &cpu.pc, 8);
  rtl_sr(id_dest->reg, &s0, 4);
  rtl_jr(&id_src->val);

  print_asm_template2(jalr);
}

make_EHelper(bne) {
  rtl_jrelop(RELOP_NE, &id_src->val, &id_src2->val, decinfo.jmp_pc);

  print_asm_template3(bne);
}

make_EHelper(beq) {
  rtl_jrelop(RELOP_EQ, &id_src->val, &id_src2->val, decinfo.jmp_pc);

  print_asm_template3(beq);
}

make_EHelper(blez) {
  rtl_jrelop(RELOP_LE, &id_src->val, &id_src2->val, decinfo.jmp_pc);

  print_asm_template3(blez);
}

make_EHelper(bltz) {
  rtl_li(&s0, 0);
  rtl_jrelop(RELOP_LT, &id_src->val, &s0, decinfo.jmp_pc);

  print_asm_template3(bltz);
}

make_EHelper(bgtz) {
  rtl_jrelop(RELOP_GT, &id_src->val, &id_src2->val, decinfo.jmp_pc);

  print_asm_template3(blez);
}

make_EHelper(bgez) {
  rtl_li(&s0, 0);
  rtl_jrelop(RELOP_GE, &id_src->val, &s0, decinfo.jmp_pc);

  print_asm_template3(bltz);
}
