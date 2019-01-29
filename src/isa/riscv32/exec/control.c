#include "cpu/exec.h"

make_EHelper(jal) {
  rtl_addi(&s0, &cpu.pc, 4);
  rtl_sr(id_dest->reg, &s0, 4);
  rtl_j(decinfo.jmp_pc);

  print_asm_template2(jal);
}

make_EHelper(jalr) {
  rtl_addi(&s0, &cpu.pc, 4);
  rtl_sr(id_dest->reg, &s0, 4);

  rtl_add(&s0, &id_src->val, &id_src2->val);
  rtl_andi(&s0, &s0, ~0x1u);
  rtl_jr(&s0);

  print_asm_template3(jalr);
}
