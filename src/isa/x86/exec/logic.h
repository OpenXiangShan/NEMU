#include "cc.h"

#ifndef __ICS_EXPORT
// dest <- and result
static inline void and_internal(DecodeExecState *s) {
  rtl_and(s, s0, ddest, dsrc1);
  rtl_update_ZFSF(s, s0, id_dest->width);
  rtl_mv(s, &cpu.CF, rz);
  rtl_mv(s, &cpu.OF, rz);
}

static inline def_EHelper(test) {
#ifdef LAZY_CC
  rtl_and(s, s0, ddest, dsrc1);
  rtl_set_lazycc(s, s0, NULL, NULL, LAZYCC_LOGIC, id_dest->width);
#else
  and_internal(s);
#endif
  print_asm_template2(test);
}

static inline def_EHelper(and) {
#ifdef LAZY_CC
  rtl_and(s, ddest, ddest, dsrc1);
  rtl_set_lazycc(s, ddest, NULL, NULL, LAZYCC_LOGIC, id_dest->width);
  operand_write(s, id_dest, ddest);
#else
  and_internal(s);
  operand_write(s, id_dest, s0);
#endif
  print_asm_template2(and);
}

static inline def_EHelper(xor) {
  rtl_xor(s, ddest, ddest, dsrc1);
#ifdef LAZY_CC
  rtl_set_lazycc(s, ddest, NULL, NULL, LAZYCC_LOGIC, id_dest->width);
#else
  rtl_update_ZFSF(s, ddest, id_dest->width);
  rtl_mv(s, &cpu.CF, rz);
  rtl_mv(s, &cpu.OF, rz);
#endif
  operand_write(s, id_dest, ddest);
  print_asm_template2(xor);
}

static inline def_EHelper(or) {
  rtl_or(s, ddest, ddest, dsrc1);
#ifdef LAZY_CC
  rtl_set_lazycc(s, ddest, NULL, NULL, LAZYCC_LOGIC, id_dest->width);
#else
  rtl_update_ZFSF(s, ddest, id_dest->width);
  rtl_mv(s, &cpu.CF, rz);
  rtl_mv(s, &cpu.OF, rz);
#endif
  operand_write(s, id_dest, ddest);
  print_asm_template2(or);
}

static inline def_EHelper(not) {
  rtl_not(s, ddest, ddest);
  operand_write(s, id_dest, ddest);
  print_asm_template1(not);
}

static inline def_EHelper(sar) {
  // if ddest == dsrc1, rtl_sar() still only use the
  // lower 5 bits of dsrc1, which do not change after
  // rtl_sext(), and it is  still sematically correct
  rtl_sext(s, ddest, ddest, id_dest->width);
  rtl_sar(s, ddest, ddest, dsrc1);
  operand_write(s, id_dest, ddest);
#ifndef LAZY_CC
  // unnecessary to update CF and OF in NEMU
  rtl_update_ZFSF(s, ddest, id_dest->width);
#endif
  print_asm_template2(sar);
}

static inline def_EHelper(shl) {
  rtl_shl(s, ddest, ddest, dsrc1);
  operand_write(s, id_dest, ddest);
#ifndef LAZY_CC
  // unnecessary to update CF and OF in NEMU
  rtl_update_ZFSF(s, ddest, id_dest->width);
#endif
  print_asm_template2(shl);
}

static inline def_EHelper(shr) {
  int count = *dsrc1 & 0x1f;
  if (count == 1) {
    rtl_msb(s, s0, ddest, id_dest->width);
    rtl_set_OF(s, s0);
  }
  if (count != 0) {
    rtl_andi(s, s1, ddest, 1);
    rtl_set_CF(s, s1);
  }
  rtl_shr(s, ddest, ddest, dsrc1);
  operand_write(s, id_dest, ddest);
  rtl_update_ZFSF(s, ddest, id_dest->width);
#ifdef LAZY_CC
  panic("TODO: implement CF and OF with lazy cc");
#endif
  print_asm_template2(shr);
}

static inline def_EHelper(rol) {
  rtl_shl(s, s0, ddest, dsrc1);
  rtl_li(s, s1, id_dest->width * 8);
  rtl_sub(s, s1, s1, dsrc1);
  rtl_shr(s, s1, ddest, s1);
  rtl_or(s, ddest, s0, s1);

  operand_write(s, id_dest, ddest);
  // unnecessary to update eflags in NEMU
  //difftest_skip_eflags(EFLAGS_MASK_ALL);
  print_asm_template2(rol);
}

static inline def_EHelper(ror) {
  rtl_shr(s, s0, ddest, dsrc1);
  rtl_li(s, s1, id_dest->width * 8);
  rtl_sub(s, s1, s1, dsrc1);
  rtl_shl(s, s1, ddest, s1);
  rtl_or(s, ddest, s0, s1);

  operand_write(s, id_dest, ddest);
  // unnecessary to update eflags in NEMU
  //difftest_skip_eflags(EFLAGS_MASK_ALL);
  print_asm_template2(ror);
}

static inline def_EHelper(shld) {
  assert(id_dest->width == 4);
  rtl_shl(s, s0, ddest, dsrc1);

  rtl_li(s, s1, 31);
  rtl_sub(s, s1, s1, dsrc1);
  // shift twice to deal with dsrc1 = 0
  // the first shift is still right even if we do not
  // mask out the high part of `dsrc1`, since we have
  //   (31 - (dsrc1 & 0x1f)) = (31 - dsrc1 % 32) = (31 - dsrc1) mod 32
  rtl_shr(s, s1, dsrc2, s1);
  rtl_shri(s, s1, s1, 1);

  rtl_or(s, ddest, s0, s1);

  operand_write(s, id_dest, ddest);
#ifndef LAZY_CC
  rtl_update_ZFSF(s, ddest, id_dest->width);
  // unnecessary to update CF and OF in NEMU
#endif
  print_asm_template3(shld);
}

static inline def_EHelper(shrd) {
  assert(id_dest->width == 4);
  rtl_shr(s, s0, ddest, dsrc1);

  rtl_li(s, s1, 31);
  rtl_sub(s, s1, s1, dsrc1);
  // shift twice to deal with dsrc1 = 0
  // the first shift is still right even if we do not
  // mask out the high part of `dsrc1`, since we have
  //   (31 - (dsrc1 & 0x1f)) = (31 - dsrc1 % 32) = (31 - dsrc1) mod 32
  rtl_shl(s, s1, dsrc2, s1);
  rtl_shli(s, s1, s1, 1);

  rtl_or(s, ddest, s0, s1);

  operand_write(s, id_dest, ddest);
#ifndef LAZY_CC
  rtl_update_ZFSF(s, ddest, id_dest->width);
  // unnecessary to update CF and OF in NEMU
#endif
  print_asm_template3(shrd);
}

static inline def_EHelper(rcr) {
  rtl_shr(s, s0, ddest, dsrc1);

  rtl_get_CF(s, s1);
  rtl_shli(s, s1, s1, 31);
  rtl_shr(s, s1, s1, dsrc1);
  rtl_shli(s, s1, s1, 1);
  rtl_or(s, s0, s0, s1);

  rtl_li(s, s1, 1);
  rtl_shl(s, s1, s1, dsrc1);
  rtl_shri(s, s1, s1, 1);
  rtl_and(s, s1, ddest, s1);
  rtl_setrelopi(s, RELOP_NE, s1, s1, 0);
  rtl_set_CF(s, s1);

  rtl_li(s, s1, id_dest->width * 8);
  rtl_sub(s, s1, s1, dsrc1);
  rtl_shl(s, s1, ddest, s1);
  rtl_shli(s, s1, s1, 1);
  rtl_or(s, ddest, s0, s1);

  operand_write(s, id_dest, ddest);
  print_asm_template2(rcr);
}

static inline def_EHelper(rcl) {
  rtl_shl(s, s0, ddest, dsrc1);

  rtl_get_CF(s, s1);
  rtl_shl(s, s1, s1, dsrc1);
  rtl_shri(s, s1, s1, 1);
  rtl_or(s, s0, s0, s1);

  rtl_li(s, s1, 0x80000000);
  rtl_shr(s, s1, s1, dsrc1);
  rtl_shli(s, s1, s1, 1);
  rtl_and(s, s1, ddest, s1);
  rtl_setrelopi(s, RELOP_NE, s1, s1, 0);
  rtl_set_CF(s, s1);

  rtl_li(s, s1, id_dest->width * 8);
  rtl_sub(s, s1, s1, dsrc1);
  rtl_shr(s, s1, ddest, s1);
  rtl_shri(s, s1, s1, 1);
  rtl_or(s, ddest, s0, s1);

  operand_write(s, id_dest, ddest);
  print_asm_template2(rcl);
}
#else
static inline def_EHelper(test) {
  TODO();
  print_asm_template2(test);
}

static inline def_EHelper(and) {
  TODO();
  print_asm_template2(and);
}

static inline def_EHelper(xor) {
  TODO();
  print_asm_template2(xor);
}

static inline def_EHelper(or) {
  TODO();
  print_asm_template2(or);
}

static inline def_EHelper(not) {
  TODO();
  print_asm_template1(not);
}

static inline def_EHelper(sar) {
  TODO();
  // unnecessary to update CF and OF in NEMU
  print_asm_template2(sar);
}

static inline def_EHelper(shl) {
  TODO();
  // unnecessary to update CF and OF in NEMU
  print_asm_template2(shl);
}

static inline def_EHelper(shr) {
  TODO();
  // unnecessary to update CF and OF in NEMU
  print_asm_template2(shr);
}

#endif

static inline def_EHelper(setcc) {
  uint32_t cc = s->opcode & 0xf;
#ifdef LAZY_CC
  rtl_lazy_setcc(s, ddest, cc);
#else
  rtl_setcc(s, ddest, cc);
#endif
  operand_write(s, id_dest, ddest);

  print_asm("set%s %s", get_cc_name(cc), id_dest->str);
}
