#include "cc.h"

static inline make_EHelper(add) {
//  TODO();
#ifdef LAZY_CC
  rtl_set_lazycc_src1(s, dsrc1);
  rtl_add(s, ddest, ddest, dsrc1);
  rtl_set_lazycc(s, ddest, NULL, NULL, LAZYCC_ADD, id_dest->width);
  operand_write(s, id_dest, ddest);
#else
  rtl_add(s, s0, ddest, dsrc1);
  rtl_update_ZFSF(s, s0, id_dest->width);
  if (id_dest->width != 4) {
    rtl_andi(s, s0, s0, 0xffffffffu >> ((4 - id_dest->width) * 8));
  }
  rtl_is_add_carry(s, s1, s0, ddest);
  rtl_set_CF(s, s1);
  rtl_is_add_overflow(s, s1, s0, ddest, dsrc1, id_dest->width);
  rtl_set_OF(s, s1);
  operand_write(s, id_dest, s0);
#endif
  print_asm_template2(add);
}

// dest <- sub result
static inline void cmp_internal(DecodeExecState *s) {
  rtl_sub(s, s0, ddest, dsrc1);
  rtl_update_ZFSF(s, s0, id_dest->width);
  rtl_is_sub_carry(s, s1, ddest, dsrc1);
  rtl_set_CF(s, s1);
  rtl_is_sub_overflow(s, s1, s0, ddest, dsrc1, id_dest->width);
  rtl_set_OF(s, s1);
}
 

static inline make_EHelper(sub) {
//  TODO();
#ifdef LAZY_CC
  rtl_set_lazycc(s, ddest, dsrc1, NULL, LAZYCC_SUB, id_dest->width);
  rtl_sub(s, ddest, ddest, dsrc1);
  operand_write(s, id_dest, ddest);
#else
  cmp_internal(s);
  operand_write(s, id_dest, s0);
#endif
  print_asm_template2(sub);
}

static inline make_EHelper(cmp) {
//  TODO();
#ifdef LAZY_CC
  rtl_set_lazycc(s, ddest, dsrc1, NULL, LAZYCC_SUB, id_dest->width);
#else
  cmp_internal(s);
#endif
  print_asm_template2(cmp);
}

static inline make_EHelper(inc) {
//  TODO();
  rtl_addi(s, ddest, ddest, 1);
#ifdef LAZY_CC
  rtl_set_lazycc(s, ddest, NULL, NULL, LAZYCC_INC, id_dest->width);
#else
  rtl_update_ZFSF(s, ddest, id_dest->width);
  rtl_setrelopi(s, RELOP_EQ, s1, ddest, 0x1u << (id_dest->width * 8 - 1));
  rtl_set_OF(s, s1);
#endif
  operand_write(s, id_dest, ddest);
  print_asm_template1(inc);
}

static inline make_EHelper(dec) {
//  TODO();
#ifdef LAZY_CC
  rtl_subi(s, ddest, ddest, 1);
  rtl_set_lazycc(s, ddest, NULL, NULL, LAZYCC_DEC, id_dest->width);
  operand_write(s, id_dest, ddest);
#else
  rtl_subi(s, s0, ddest, 1);
  rtl_update_ZFSF(s, s0, id_dest->width);
  rtl_setrelopi(s, RELOP_EQ, s1, ddest, 0x1u << (id_dest->width * 8 - 1));
  rtl_set_OF(s, s1);
  operand_write(s, id_dest, s0);
#endif
  print_asm_template1(dec);
}

static inline make_EHelper(neg) {
//  TODO();
#ifdef LAZY_CC
  rtl_sub(s, ddest, rz, ddest);
  rtl_set_lazycc(s, ddest, NULL, NULL, LAZYCC_NEG, id_dest->width);
  operand_write(s, id_dest, ddest);
#else
  rtl_sub(s, s0, rz, ddest);
  rtl_update_ZFSF(s, s0, id_dest->width);
  rtl_setrelopi(s, RELOP_NE, s1, ddest, 0);
  rtl_set_CF(s, s1);
  rtl_setrelopi(s, RELOP_EQ, s1, ddest, 0x1u << (id_dest->width * 8 - 1));
  rtl_set_OF(s, s1);
  operand_write(s, id_dest, s0);
#endif
  print_asm_template1(neg);
}

static inline make_EHelper(adc) {
#ifdef LAZY_CC
  rtl_lazy_setcc(s, s0, CC_B); // reading CC_B is to read CF
  rtl_add(s, s0, dsrc1, s0);
  rtl_set_lazycc_src2(s, dsrc1);
  rtl_add(s, ddest, ddest, s0);
  rtl_set_lazycc(s, ddest, s0, NULL, LAZYCC_ADC, id_dest->width);
  operand_write(s, id_dest, ddest);
#else
  rtl_get_CF(s, s0);
  rtl_add(s, s0, dsrc1, s0);
  rtl_add(s, s1, ddest, s0);
  rtl_update_ZFSF(s, s1, id_dest->width);
  rtl_is_add_overflow(s, s2, s1, ddest, dsrc1, id_dest->width);
  rtl_set_OF(s, s2);
  if (id_dest->width != 4) {
    rtl_andi(s, s1, s1, 0xffffffffu >> ((4 - id_dest->width) * 8));
  }
  rtl_is_add_carry(s, s2, s1, s0);
  rtl_is_add_carry(s, s0, s0, dsrc1);
  rtl_or(s, s0, s0, s2);
  rtl_set_CF(s, s0);
  operand_write(s, id_dest, s1);
#endif
  print_asm_template2(adc);
}

static inline make_EHelper(sbb) {
#ifdef LAZY_CC
  rtl_lazy_setcc(s, s0, CC_B); // reading CC_B is to read CF
  rtl_add(s, s0, dsrc1, s0);
  rtl_set_lazycc_src2(s, dsrc1);
  rtl_set_lazycc_src1(s, ddest);
  rtl_sub(s, ddest, ddest, s0);
  rtl_set_lazycc(s, ddest, NULL, NULL, LAZYCC_SBB, id_dest->width);
  operand_write(s, id_dest, ddest);
#else
  rtl_get_CF(s, s0);
  rtl_add(s, s0, dsrc1, s0);
  rtl_sub(s, s1, ddest, s0);
  rtl_update_ZFSF(s, s1, id_dest->width);
  rtl_is_sub_overflow(s, s2, s1, ddest, dsrc1, id_dest->width);
  rtl_set_OF(s, s2);
  rtl_is_add_carry(s, s2, s0, dsrc1);
  rtl_is_sub_carry(s, s0, ddest, s0);
  rtl_or(s, s0, s0, s2);
  rtl_set_CF(s, s0);
  operand_write(s, id_dest, s1);
#endif
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
  switch (id_dest->width) {
    case 1:
      rtl_lr(s, s0, R_EAX, 1);
      rtl_imul_lo(s, s1, ddest, s0);
      rtl_sr(s, R_AX, s1, 2);
      break;
    case 2:
      rtl_lr(s, s0, R_EAX, 2);
      rtl_imul_lo(s, s1, ddest, s0);
      rtl_sr(s, R_AX, s1, 2);
      rtl_shri(s, s1, s1, 16);
      rtl_sr(s, R_DX, s1, 2);
      break;
    case 4:
      ; rtlreg_t *pdest = ddest;
      if (ddest == &cpu.edx) {
        rtl_mv(s, s0, ddest);
        pdest = s0;
      }
      rtl_imul_hi(s, &cpu.edx, pdest, &cpu.eax);
      rtl_imul_lo(s, &cpu.eax, pdest, &cpu.eax);
      break;
    default: assert(0);
  }

  //difftest_skip_eflags(EFLAGS_MASK_ALL);
  print_asm_template1(imul);
}

// imul with two operands
static inline make_EHelper(imul2) {
  rtl_sext(s, dsrc1, dsrc1, id_src1->width);
  rtl_sext(s, ddest, ddest, id_dest->width);

  rtl_imul_lo(s, ddest, ddest, dsrc1);
  operand_write(s, id_dest, ddest);

  //difftest_skip_eflags(EFLAGS_MASK_ALL);
  print_asm_template2(imul);
}

// imul with three operands
static inline make_EHelper(imul3) {
  rtl_sext(s, dsrc1, dsrc1, id_src1->width);
  rtl_sext(s, dsrc2, dsrc2, id_src1->width);

  rtl_imul_lo(s, ddest, dsrc2, dsrc1);
  operand_write(s, id_dest, ddest);

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
      ; rtlreg_t *pdest = ddest;
      if (ddest == &cpu.eax) pdest = s0;
      rtl_mv(s, s0, &cpu.eax);
      rtl_div64_q(s, &cpu.eax, &cpu.edx, s0, pdest);
      rtl_div64_r(s, &cpu.edx, &cpu.edx, s0, pdest);
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
      ; rtlreg_t *pdest = ddest;
      if (ddest == &cpu.eax) pdest = s0;
      rtl_mv(s, s0, &cpu.eax);
      rtl_idiv64_q(s, &cpu.eax, &cpu.edx, s0, pdest);
      rtl_idiv64_r(s, &cpu.edx, &cpu.edx, s0, pdest);
      break;
    default: assert(0);
  }

  //difftest_skip_eflags(EFLAGS_MASK_ALL);
  print_asm_template1(idiv);
}
