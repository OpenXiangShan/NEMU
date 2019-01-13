#include "cpu/exec.h"

make_EHelper(j) {
  rtl_j(decinfo.jmp_pc);

  print_asm_template1(j);
}

make_EHelper(jal) {
  rtl_addi(&t0, &cpu.pc, 8);
  rtl_sr(31, &t0, 4);
  rtl_j(decinfo.jmp_pc);

  print_asm_template1(jal);
}

make_EHelper(jr) {
  rtl_jr(&id_src->val);

  print_asm_template1(jr);
}

make_EHelper(jalr) {
  rtl_addi(&t0, &cpu.pc, 8);
  rtl_sr(id_dest->reg, &t0, 4);
  rtl_jr(&id_src->val);

  print_asm_template1(jalr);
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
