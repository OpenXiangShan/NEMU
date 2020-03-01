#include "cpu/exec.h"
#include "cc.h"

// dest <- and result
static inline void and_internal() {
  rtl_and(&s0, &id_dest->val, &id_src->val);
  rtl_update_ZFSF(&s0, id_dest->width);
  rtl_li(&cpu.CF, 0);
  rtl_li(&cpu.OF, 0);
}

static make_EHelper(test) {
  and_internal();
  print_asm_template2(test);
}

static make_EHelper(and) {
  and_internal();
  operand_write(id_dest, &s0);
  print_asm_template2(and);
}

static make_EHelper(xor) {
  rtl_xor(&s0, &id_dest->val, &id_src->val);
  operand_write(id_dest, &s0);
  rtl_update_ZFSF(&s0, id_dest->width);
  rtl_li(&cpu.CF, 0);
  rtl_li(&cpu.OF, 0);
  print_asm_template2(xor);
}

static make_EHelper(or) {
  rtl_or(&s0, &id_dest->val, &id_src->val);
  operand_write(id_dest, &s0);
  rtl_update_ZFSF(&s0, id_dest->width);
  rtl_li(&cpu.CF, 0);
  rtl_li(&cpu.OF, 0);
  print_asm_template2(or);
}

static make_EHelper(sar) {
  rtl_sext(&s0, &id_dest->val, id_dest->width);
  rtl_sar(&s0, &s0, &id_src->val);
  operand_write(id_dest, &s0);
  rtl_update_ZFSF(&s0, id_dest->width);
  // unnecessary to update CF and OF in NEMU
  //difftest_skip_eflags(EFLAGS_MASK_CF | EFLAGS_MASK_OF);
  print_asm_template2(sar);
}

static make_EHelper(shl) {
  rtl_shl(&s0, &id_dest->val, &id_src->val);
  operand_write(id_dest, &s0);
  rtl_update_ZFSF(&s0, id_dest->width);
  // unnecessary to update CF and OF in NEMU
  //difftest_skip_eflags(EFLAGS_MASK_CF | EFLAGS_MASK_OF | EFLAGS_MASK_ZF);
  print_asm_template2(shl);
}

static make_EHelper(shr) {
  rtl_shr(&s0, &id_dest->val, &id_src->val);
  operand_write(id_dest, &s0);
  rtl_update_ZFSF(&s0, id_dest->width);
  // unnecessary to update CF and OF in NEMU
  //difftest_skip_eflags(EFLAGS_MASK_CF | EFLAGS_MASK_OF);
  print_asm_template2(shr);
}

static make_EHelper(rol) {
  rtl_shl(&s0, &id_dest->val, &id_src->val);
  rtl_li(&s1, id_dest->width * 8);
  rtl_sub(&s1, &s1, &id_src->val);
  rtl_shr(&s1, &id_dest->val, &s1);
  rtl_or(&s1, &s0, &s1);

  operand_write(id_dest, &s1);
  // unnecessary to update eflags in NEMU
  //difftest_skip_eflags(EFLAGS_MASK_ALL);
  print_asm_template2(rol);
}

static make_EHelper(setcc) {
  uint32_t cc = decinfo.opcode & 0xf;

  rtl_setcc(&s0, cc);
  operand_write(id_dest, &s0);

  print_asm("set%s %s", get_cc_name(cc), id_dest->str);
}

static make_EHelper(not) {
  rtl_not(&s0, &id_dest->val);
  operand_write(id_dest, &s0);

  print_asm_template1(not);
}

static make_EHelper(shld) {
  rtl_shl(&s0, &id_dest->val, &id_src->val);

  rtl_li(&s1, 32);
  rtl_sub(&s1, &s1, &id_src->val);
  rtl_shr(&s1, &id_src2->val, &s1);

  rtl_or(&s0, &s0, &s1);

  operand_write(id_dest, &s0);
  rtl_update_ZFSF(&s0, id_dest->width);
  // unnecessary to update CF and OF in NEMU
  //difftest_skip_eflags(EFLAGS_MASK_CF | EFLAGS_MASK_OF);
  print_asm_template3(shld);
}

static make_EHelper(shrd) {
  rtl_shr(&s0, &id_dest->val, &id_src->val);

  rtl_li(&s1, 32);
  rtl_sub(&s1, &s1, &id_src->val);
  rtl_shl(&s1, &id_src2->val, &s1);

  rtl_or(&s0, &s0, &s1);

  operand_write(id_dest, &s0);
  rtl_update_ZFSF(&s0, id_dest->width);
  // unnecessary to update CF and OF in NEMU
  //difftest_skip_eflags(EFLAGS_MASK_CF | EFLAGS_MASK_OF);
  print_asm_template3(shrd);
}
