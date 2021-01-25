#ifndef __ICS_EXPORT
#include <cpu/difftest.h>

static inline void difftest_skip_delay_slot() {
#ifndef __DIFF_REF_NEMU__
  difftest_skip_dut(2, 1);
#endif
}

static inline def_EHelper(j) {
  difftest_skip_delay_slot();
  rtl_j(s, id_dest->imm);
  print_asm_template1(j);
}

static inline def_EHelper(jal) {
  difftest_skip_delay_slot();
  word_t ra = cpu.pc + 8;
  rtl_j(s, id_dest->imm);
  rtl_li(s, &reg_l(31), ra);
  print_asm_template1(jal);
}

static inline def_EHelper(jr) {
  difftest_skip_delay_slot();
  rtl_jr(s, dsrc1);
  print_asm_template2(jr);
}

static inline def_EHelper(jalr) {
  difftest_skip_delay_slot();
  word_t ra = cpu.pc + 8;
  rtl_jr(s, dsrc1);
  rtl_li(s, ddest, ra);
  print_asm_template2(jalr);
}

static inline def_EHelper(bne) {
  difftest_skip_delay_slot();
  rtl_jrelop(s, RELOP_NE, dsrc1, dsrc2, id_dest->imm);
  print_asm_template3(bne);
}

static inline def_EHelper(beq) {
  difftest_skip_delay_slot();
  rtl_jrelop(s, RELOP_EQ, dsrc1, dsrc2, id_dest->imm);
  print_asm_template3(beq);
}

static inline def_EHelper(blez) {
  difftest_skip_delay_slot();
  rtl_jrelop(s, RELOP_LE, dsrc1, dsrc2, id_dest->imm);
  print_asm_template3(blez);
}

static inline def_EHelper(bltz) {
  difftest_skip_delay_slot();
  rtl_jrelop(s, RELOP_LT, dsrc1, rz, id_dest->imm);
  print_asm_template3(bltz);
}

static inline def_EHelper(bgtz) {
  difftest_skip_delay_slot();
  rtl_jrelop(s, RELOP_GT, dsrc1, dsrc2, id_dest->imm);
  print_asm_template3(blez);
}

static inline def_EHelper(bgez) {
  difftest_skip_delay_slot();
  rtl_jrelop(s, RELOP_GE, dsrc1, rz, id_dest->imm);
  print_asm_template3(bltz);
}
#endif
