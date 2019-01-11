#include "cpu/exec.h"
#include "cc.h"

// dest <- and result
static inline void and_internal(rtlreg_t *dest) {
  rtl_and(dest, &id_dest->val, &id_src->val);
  rtl_update_ZFSF(dest, id_dest->width);
  rtl_li(&cpu.CF, 0);
  rtl_li(&cpu.OF, 0);
}

make_EHelper(test) {
  and_internal(&t0);
  print_asm_template2(test);
}

make_EHelper(and) {
  and_internal(&t0);
  operand_write(id_dest, &t0);
  print_asm_template2(and);
}

make_EHelper(xor) {
  rtl_xor(&t2, &id_dest->val, &id_src->val);
  operand_write(id_dest, &t2);
  rtl_update_ZFSF(&t2, id_dest->width);
  rtl_li(&cpu.CF, 0);
  rtl_li(&cpu.OF, 0);
  print_asm_template2(xor);
}

make_EHelper(or) {
  rtl_or(&t2, &id_dest->val, &id_src->val);
  operand_write(id_dest, &t2);
  rtl_update_ZFSF(&t2, id_dest->width);
  rtl_li(&cpu.CF, 0);
  rtl_li(&cpu.OF, 0);
  print_asm_template2(or);
}

make_EHelper(sar) {
  rtl_sext(&t2, &id_dest->val, id_dest->width);
  rtl_sar(&t2, &t2, &id_src->val);
  operand_write(id_dest, &t2);
  rtl_update_ZFSF(&t2, id_dest->width);
  // unnecessary to update CF and OF in NEMU
  //difftest_skip_eflags(EFLAGS_MASK_CF | EFLAGS_MASK_OF);
  print_asm_template2(sar);
}

make_EHelper(shl) {
  rtl_shl(&t2, &id_dest->val, &id_src->val);
  operand_write(id_dest, &t2);
  rtl_update_ZFSF(&t2, id_dest->width);
  // unnecessary to update CF and OF in NEMU
  //difftest_skip_eflags(EFLAGS_MASK_CF | EFLAGS_MASK_OF | EFLAGS_MASK_ZF);
  print_asm_template2(shl);
}

make_EHelper(shr) {
  rtl_shr(&t2, &id_dest->val, &id_src->val);
  operand_write(id_dest, &t2);
  rtl_update_ZFSF(&t2, id_dest->width);
  // unnecessary to update CF and OF in NEMU
  //difftest_skip_eflags(EFLAGS_MASK_CF | EFLAGS_MASK_OF);
  print_asm_template2(shr);
}

make_EHelper(rol) {
  rtl_shl(&t1, &id_dest->val, &id_src->val);
  rtl_li(&t2, id_dest->width * 8);
  rtl_sub(&t2, &t2, &id_src->val);
  rtl_shr(&t2, &id_dest->val, &t2);
  rtl_or(&t2, &t1, &t2);

  operand_write(id_dest, &t2);
  // unnecessary to update eflags in NEMU
  //difftest_skip_eflags(EFLAGS_MASK_ALL);
  print_asm_template2(rol);
}

make_EHelper(setcc) {
  uint32_t cc = decinfo.opcode & 0xf;

  rtl_setcc(&t2, cc);
  operand_write(id_dest, &t2);

  print_asm("set%s %s", get_cc_name(cc), id_dest->str);
}

make_EHelper(not) {
  rtl_not(&t0, &id_dest->val);
  operand_write(id_dest, &t0);

  print_asm_template1(not);
}

make_EHelper(shld) {
  rtl_shl(&t2, &id_dest->val, &id_src->val);

  rtl_li(&t0, 32);
  rtl_sub(&t0, &t0, &id_src->val);
  rtl_shr(&t0, &id_src2->val, &t0);

  rtl_or(&t2, &t2, &t0);

  operand_write(id_dest, &t2);
  rtl_update_ZFSF(&t2, id_dest->width);
  // unnecessary to update CF and OF in NEMU
  //difftest_skip_eflags(EFLAGS_MASK_CF | EFLAGS_MASK_OF);
  print_asm_template3(shld);
}

make_EHelper(shrd) {
  rtl_shr(&t2, &id_dest->val, &id_src->val);

  rtl_li(&t0, 32);
  rtl_sub(&t0, &t0, &id_src->val);
  rtl_shl(&t0, &id_src2->val, &t0);

  rtl_or(&t2, &t2, &t0);

  operand_write(id_dest, &t2);
  rtl_update_ZFSF(&t2, id_dest->width);
  // unnecessary to update CF and OF in NEMU
  //difftest_skip_eflags(EFLAGS_MASK_CF | EFLAGS_MASK_OF);
  print_asm_template3(shrd);
}
