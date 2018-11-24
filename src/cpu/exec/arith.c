#include "cpu/exec.h"

make_EHelper(add) {
//  TODO();
  rtl_add(&t2, &id_dest->val, &id_src->val);
  operand_write(id_dest, &t2);

  rtl_update_ZFSF(&t2, id_dest->width);

  rtl_is_add_carry(&cpu.CF, &t2, &id_dest->val);
  rtl_is_add_overflow(&cpu.OF, &t2, &id_dest->val, &id_src->val, id_dest->width);

  print_asm_template2(add);
}

// dest <- sub result
static inline void cmp_internal(rtlreg_t *dest) {
  rtl_sub(dest, &id_dest->val, &id_src->val);

  rtl_update_ZFSF(dest, id_dest->width);

  rtl_is_sub_carry(&cpu.CF, dest, &id_dest->val);
  rtl_is_sub_overflow(&cpu.OF, dest, &id_dest->val, &id_src->val, id_dest->width);
}
 

make_EHelper(sub) {
//  TODO();
  cmp_internal(&t2);

  operand_write(id_dest, &t2);

  print_asm_template2(sub);
}

make_EHelper(cmp) {
//  TODO();
  cmp_internal(&t2);

  print_asm_template2(cmp);
}

make_EHelper(inc) {
//  TODO();
  rtl_addi(&t2, &id_dest->val, 1);
  operand_write(id_dest, &t2);

  rtl_update_ZFSF(&t2, id_dest->width);

  rtl_setrelopi(RELOP_EQ, &cpu.OF, &t2, 0x80000000);

  print_asm_template1(inc);
}

make_EHelper(dec) {
//  TODO();
  rtl_subi(&t2, &id_dest->val, 1);
  operand_write(id_dest, &t2);

  rtl_update_ZFSF(&t2, id_dest->width);

  rtl_setrelopi(RELOP_EQ, &cpu.OF, &t2, 0x7fffffff);

  print_asm_template1(dec);
}

make_EHelper(neg) {
//  TODO();
  rtl_li(&t0, 0);
  rtl_sub(&t2, &t0, &id_dest->val);

  rtl_update_ZFSF(&t2, id_dest->width);

  rtl_setrelopi(RELOP_NE, &cpu.CF, &id_dest->val, 0);
  rtl_setrelopi(RELOP_EQ, &cpu.OF, &id_dest->val, 0x80000000);

  operand_write(id_dest, &t2);

  print_asm_template1(neg);
}

make_EHelper(adc) {
  // t2 = dest + src
  rtl_add(&t2, &id_dest->val, &id_src->val);
  // t3 = t2 + CF
  rtl_add(&t3, &t2, &cpu.CF);

  // update OF
  rtl_is_add_overflow(&cpu.OF, &t3, &id_dest->val, &id_src->val, id_dest->width);

  // update CF
  rtl_is_add_carry(&t0, &t2, &id_dest->val);
  rtl_is_add_carry(&t1, &t3, &t2);
  rtl_or(&cpu.CF, &t0, &t1);

  rtl_update_ZFSF(&t3, id_dest->width);
  operand_write(id_dest, &t3);

  print_asm_template2(adc);
}

make_EHelper(sbb) {
  // t2 = dest - src
  rtl_sub(&t2, &id_dest->val, &id_src->val);
  // t3 = t2 - CF
  rtl_sub(&t3, &t2, &cpu.CF);

  // update OF
  rtl_is_sub_overflow(&cpu.OF, &t3, &id_dest->val, &id_src->val, id_dest->width);

  // update CF
  rtl_is_sub_carry(&t0, &t2, &id_dest->val);
  rtl_is_sub_carry(&t1, &t3, &t2);
  rtl_or(&cpu.CF, &t0, &t1);

  rtl_update_ZFSF(&t3, id_dest->width);
  operand_write(id_dest, &t3);

  print_asm_template2(sbb);
}

make_EHelper(mul) {
  rtl_lr(&t0, R_EAX, id_dest->width);
  rtl_mul_lo(&t1, &id_dest->val, &t0);

  switch (id_dest->width) {
    case 1:
      rtl_sr(R_AX, &t1, 2);
      break;
    case 2:
      rtl_sr(R_AX, &t1, 2);
      rtl_shri(&t1, &t1, 16);
      rtl_sr(R_DX, &t1, 2);
      break;
    case 4:
      rtl_mul_hi(&t2, &id_dest->val, &t0);
      rtl_sr(R_EDX, &t2, 4);
      rtl_sr(R_EAX, &t1, 4);
      break;
    default: assert(0);
  }

  difftest_skip_eflags(EFLAGS_MASK_ALL);
  print_asm_template1(mul);
}

// imul with one operand
make_EHelper(imul1) {
  rtl_lr(&t0, R_EAX, id_dest->width);
  rtl_imul_lo(&t1, &id_dest->val, &t0);

  switch (id_dest->width) {
    case 1:
      rtl_sr(R_AX, &t1, 2);
      break;
    case 2:
      rtl_sr(R_AX, &t1, 2);
      rtl_shri(&t1, &t1, 16);
      rtl_sr(R_DX, &t1, 2);
      break;
    case 4:
      rtl_imul_hi(&t2, &id_dest->val, &t0);
      rtl_sr(R_EDX, &t2, 4);
      rtl_sr(R_EAX, &t1, 4);
      break;
    default: assert(0);
  }

  difftest_skip_eflags(EFLAGS_MASK_ALL);
  print_asm_template1(imul);
}

// imul with two operands
make_EHelper(imul2) {
  rtl_sext(&t0, &id_src->val, id_src->width);
  rtl_sext(&t1, &id_dest->val, id_dest->width);

  rtl_imul_lo(&t2, &t1, &t0);
  operand_write(id_dest, &t2);

  difftest_skip_eflags(EFLAGS_MASK_ALL);
  print_asm_template2(imul);
}

// imul with three operands
make_EHelper(imul3) {
  rtl_sext(&t0, &id_src->val, id_src->width);
  rtl_sext(&t1, &id_src2->val, id_src->width);
  rtl_sext(&id_dest->val, &id_dest->val, id_dest->width);

  rtl_imul_lo(&t2, &t1, &t0);
  operand_write(id_dest, &t2);

  difftest_skip_eflags(EFLAGS_MASK_ALL);
  print_asm_template3(imul);
}

make_EHelper(div) {
  switch (id_dest->width) {
    case 1:
      rtl_lr(&t0, R_AX, 2);
      rtl_div_q(&t2, &t0, &id_dest->val);
      rtl_div_r(&t3, &t0, &id_dest->val);
      rtl_sr(R_AL, &t2, 1);
      rtl_sr(R_AH, &t3, 1);
      break;
    case 2:
      rtl_lr(&t0, R_AX, 2);
      rtl_lr(&t1, R_DX, 2);
      rtl_shli(&t1, &t1, 16);
      rtl_or(&t0, &t0, &t1);
      rtl_div_q(&t2, &t0, &id_dest->val);
      rtl_div_r(&t3, &t0, &id_dest->val);
      rtl_sr(R_AX, &t2, 2);
      rtl_sr(R_DX, &t3, 2);
      break;
    case 4:
      rtl_lr(&t0, R_EAX, 4);
      rtl_lr(&t1, R_EDX, 4);
      rtl_div64_q(&cpu.eax, &t1, &t0, &id_dest->val);
      rtl_div64_r(&cpu.edx, &t1, &t0, &id_dest->val);
      break;
    default: assert(0);
  }

  difftest_skip_eflags(EFLAGS_MASK_ALL);
  print_asm_template1(div);
}

make_EHelper(idiv) {
  switch (id_dest->width) {
    case 1:
      rtl_lr(&t0, R_AX, 2);
      rtl_idiv_q(&t2, &t0, &id_dest->val);
      rtl_idiv_r(&t3, &t0, &id_dest->val);
      rtl_sr(R_AL, &t2, 1);
      rtl_sr(R_AH, &t3, 1);
      break;
    case 2:
      rtl_lr(&t0, R_AX, 2);
      rtl_lr(&t1, R_DX, 2);
      rtl_shli(&t1, &t1, 16);
      rtl_or(&t0, &t0, &t1);
      rtl_idiv_q(&t2, &t0, &id_dest->val);
      rtl_idiv_r(&t3, &t0, &id_dest->val);
      rtl_sr(R_AX, &t2, 2);
      rtl_sr(R_DX, &t3, 2);
      break;
    case 4:
      rtl_lr(&t0, R_EAX, 4);
      rtl_lr(&t1, R_EDX, 4);
      rtl_idiv64_q(&cpu.eax, &t1, &t0, &id_dest->val);
      rtl_idiv64_r(&cpu.edx, &t1, &t0, &id_dest->val);
      break;
    default: assert(0);
  }

  difftest_skip_eflags(EFLAGS_MASK_ALL);
  print_asm_template1(idiv);
}
