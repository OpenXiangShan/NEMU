#include "cc.h"

static inline make_EHelper(add) {
//  TODO();
  rtl_add(s, s0, ddest, dsrc1);
#ifdef LAZY_CC
  rtl_set_lazycc(s, s0, dsrc1, ddest, LAZYCC_ADD, id_dest->width);
#else
  rtl_update_ZFSF(s, s0, id_dest->width);
  if (id_dest->width != 4) {
    rtl_andi(s, s0, s0, 0xffffffffu >> ((4 - id_dest->width) * 8));
  }
  rtl_is_add_carry(s, s1, s0, ddest);
  rtl_set_CF(s, s1);
  rtl_is_add_overflow(s, s1, s0, ddest, dsrc1, id_dest->width);
  rtl_set_OF(s, s1);
#endif
  operand_write(s, id_dest, s0);
  print_asm_template2(add);
}

// dest <- sub result
static inline void cmp_internal(DecodeExecState *s) {
  rtl_sub(s, s0, ddest, dsrc1);

#ifdef LAZY_CC
  rtl_set_lazycc(s, s0, ddest, dsrc1, LAZYCC_SUB, id_dest->width);
#else
  rtl_update_ZFSF(s, s0, id_dest->width);

  if (id_dest->width != 4) {
    rtl_andi(s, s0, s0, 0xffffffffu >> ((4 - id_dest->width) * 8));
  }
  rtl_is_sub_carry(s, s1, s0, ddest);
  rtl_set_CF(s, s1);
  rtl_is_sub_overflow(s, s1, s0, ddest, dsrc1, id_dest->width);
  rtl_set_OF(s, s1);
#endif
}
 

static inline make_EHelper(sub) {
//  TODO();
  cmp_internal(s);
  operand_write(s, id_dest, s0);
  print_asm_template2(sub);
}

static inline make_EHelper(cmp) {
//  TODO();
  cmp_internal(s);
  print_asm_template2(cmp);
}

static inline make_EHelper(inc) {
//  TODO();
  rtl_addi(s, s0, ddest, 1);
#ifdef LAZY_CC
  rtl_set_lazycc(s, s0, ddest, NULL, LAZYCC_INC, id_dest->width);
#else
  rtl_update_ZFSF(s, s0, id_dest->width);
  rtl_setrelopi(s, RELOP_EQ, s1, s0, 0x80000000);
  rtl_set_OF(s, s1);
#endif
  operand_write(s, id_dest, s0);
  print_asm_template1(inc);
}

static inline make_EHelper(dec) {
//  TODO();
  rtl_subi(s, s0, ddest, 1);
#ifdef LAZY_CC
  rtl_set_lazycc(s, s0, ddest, NULL, LAZYCC_DEC, id_dest->width);
#else
  rtl_update_ZFSF(s, s0, id_dest->width);
  rtl_setrelopi(s, RELOP_EQ, s1, s0, 0x7fffffff);
  rtl_set_OF(s, s1);
#endif
  operand_write(s, id_dest, s0);
  print_asm_template1(dec);
}

static inline make_EHelper(neg) {
//  TODO();
  rtl_sub(s, s0, rz, ddest);
#ifdef LAZY_CC
  rtl_set_lazycc(s, s0, NULL, ddest, LAZYCC_NEG, id_dest->width);
#else
  rtl_update_ZFSF(s, s0, id_dest->width);
  rtl_setrelopi(s, RELOP_NE, s1, ddest, 0);
  rtl_set_CF(s, s1);
  rtl_setrelopi(s, RELOP_EQ, s1, ddest, 0x80000000);
  rtl_set_OF(s, s1);
#endif
  operand_write(s, id_dest, s0);
  print_asm_template1(neg);
}

static inline make_EHelper(adc) {
#ifdef LAZY_CC
  rtl_lazy_setcc(s, s1, CC_B); // reading CC_B is to read CF
#else
  rtl_get_CF(s, s1);
#endif
  rtl_add(s, s0, dsrc1, s0);
  rtl_add(s, s1, ddest, s0);

#ifdef LAZY_CC
  rtl_set_lazycc(s, s1, dsrc1, ddest, LAZYCC_ADC, id_dest->width);
#else
  if (id_dest->width != 4) {
    rtl_andi(s, s1, s1, 0xffffffffu >> ((4 - id_dest->width) * 8));
  }
  rtl_update_ZFSF(s, s1, id_dest->width);
  rtl_is_add_overflow(s, s0, s1, ddest, s0, id_dest->width);
  rtl_set_OF(s, s0);
  rtl_is_add_carry(s, s0, s1, ddest);
  rtl_set_CF(s, s0);
#endif
  operand_write(s, id_dest, s1);
  print_asm_template2(adc);
}

static inline make_EHelper(sbb) {
#ifdef LAZY_CC
  rtl_lazy_setcc(s, s0, CC_B); // reading CC_B is to read CF
#else
  rtl_get_CF(s, s0);
#endif
  rtl_add(s, s0, dsrc1, s0);
  rtl_sub(s, s1, ddest, s0);

#ifdef LAZY_CC
  rtl_set_lazycc(s, s1, ddest, s0, LAZYCC_SBB, id_dest->width);
#else
  if (id_dest->width != 4) {
    rtl_andi(s, s1, s1, 0xffffffffu >> ((4 - id_dest->width) * 8));
  }
  rtl_update_ZFSF(s, s1, id_dest->width);
  rtl_is_sub_overflow(s, s0, s1, ddest, s0, id_dest->width);
  rtl_set_OF(s, s0);
  rtl_is_sub_carry(s, s0, s1, ddest);
  rtl_set_CF(s, s0);
#endif
  operand_write(s, id_dest, s1);
  print_asm_template2(sbb);
}

static inline make_EHelper(mul) {
  rtl_lr(s, s0, R_EAX, id_dest->width);
  rtl_mul_lo(s, s1, ddest, s0);

  switch (id_dest->width) {
    case 1:
      rtl_sr(s, R_AX, s1, 2);
      break;
    case 2:
      rtl_sr(s, R_AX, s1, 2);
      rtl_shri(s, s1, s1, 16);
      rtl_sr(s, R_DX, s1, 2);
      break;
    case 4:
      rtl_mul_hi(s, s0, ddest, s0);
      rtl_sr(s, R_EDX, s0, 4);
      rtl_sr(s, R_EAX, s1, 4);
      break;
    default: assert(0);
  }

  //difftest_skip_eflags(EFLAGS_MASK_ALL);
  print_asm_template1(mul);
}

// imul with one operand
static inline make_EHelper(imul1) {
  rtl_lr(s, s0, R_EAX, id_dest->width);
  rtl_imul_lo(s, s1, ddest, s0);

  switch (id_dest->width) {
    case 1:
      rtl_sr(s, R_AX, s1, 2);
      break;
    case 2:
      rtl_sr(s, R_AX, s1, 2);
      rtl_shri(s, s1, s1, 16);
      rtl_sr(s, R_DX, s1, 2);
      break;
    case 4:
      rtl_imul_hi(s, s0, ddest, s0);
      rtl_sr(s, R_EDX, s0, 4);
      rtl_sr(s, R_EAX, s1, 4);
      break;
    default: assert(0);
  }

  //difftest_skip_eflags(EFLAGS_MASK_ALL);
  print_asm_template1(imul);
}

// imul with two operands
static inline make_EHelper(imul2) {
  rtl_sext(s, s0, dsrc1, id_src1->width);
  rtl_sext(s, s1, ddest, id_dest->width);

  rtl_imul_lo(s, s0, s1, s0);
  operand_write(s, id_dest, s0);

  //difftest_skip_eflags(EFLAGS_MASK_ALL);
  print_asm_template2(imul);
}

// imul with three operands
static inline make_EHelper(imul3) {
  rtl_sext(s, s0, dsrc1, id_src1->width);
  rtl_sext(s, s1, dsrc2, id_src1->width);

  rtl_imul_lo(s, s0, s1, s0);
  operand_write(s, id_dest, s0);

  //difftest_skip_eflags(EFLAGS_MASK_ALL);
  print_asm_template3(imul);
}

static inline make_EHelper(div) {
  switch (id_dest->width) {
    case 1:
      rtl_lr(s, s0, R_AX, 2);
      rtl_div_q(s, s1, s0, ddest);
      rtl_sr(s, R_AL, s1, 1);
      rtl_div_r(s, s1, s0, ddest);
      rtl_sr(s, R_AH, s1, 1);
      break;
    case 2:
      rtl_lr(s, s0, R_AX, 2);
      rtl_lr(s, s1, R_DX, 2);
      rtl_shli(s, s1, s1, 16);
      rtl_or(s, s0, s0, s1);
      rtl_div_q(s, s1, s0, ddest);
      rtl_sr(s, R_AX, s1, 2);
      rtl_div_r(s, s1, s0, ddest);
      rtl_sr(s, R_DX, s1, 2);
      break;
    case 4:
      rtl_lr(s, s0, R_EAX, 4);
      rtl_lr(s, s1, R_EDX, 4);
      rtl_div64_q(s, &cpu.eax, s1, s0, ddest);
      rtl_div64_r(s, &cpu.edx, s1, s0, ddest);
      break;
    default: assert(0);
  }

  //difftest_skip_eflags(EFLAGS_MASK_ALL);
  print_asm_template1(div);
}

static inline make_EHelper(idiv) {
  switch (id_dest->width) {
    case 1:
      rtl_lr(s, s0, R_AX, 2);
      rtl_idiv_q(s, s1, s0, ddest);
      rtl_sr(s, R_AL, s1, 1);
      rtl_idiv_r(s, s1, s0, ddest);
      rtl_sr(s, R_AH, s1, 1);
      break;
    case 2:
      rtl_lr(s, s0, R_AX, 2);
      rtl_lr(s, s1, R_DX, 2);
      rtl_shli(s, s1, s1, 16);
      rtl_or(s, s0, s0, s1);
      rtl_idiv_q(s, s1, s0, ddest);
      rtl_sr(s, R_AX, s1, 2);
      rtl_idiv_r(s, s1, s0, ddest);
      rtl_sr(s, R_DX, s1, 2);
      break;
    case 4:
      rtl_lr(s, s0, R_EAX, 4);
      rtl_lr(s, s1, R_EDX, 4);
      rtl_idiv64_q(s, &cpu.eax, s1, s0, ddest);
      rtl_idiv64_r(s, &cpu.edx, s1, s0, ddest);
      break;
    default: assert(0);
  }

  //difftest_skip_eflags(EFLAGS_MASK_ALL);
  print_asm_template1(idiv);
}
