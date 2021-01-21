#include "../local-include/intr.h"

static inline def_EHelper(lr) {
  rtl_lm(s, s0, dsrc1, 0, s->width);
  return_on_mem_ex();
  cpu.lr_addr = *dsrc1;
  rtl_sext(s, ddest, s0, s->width);

  print_asm_template3(lr);
}

static inline def_EHelper(sc) {
  // should check overlapping instead of equality
  if (cpu.lr_addr == *dsrc1) {
    rtl_sm(s, dsrc1, 0, dsrc2, s->width);
    return_on_mem_ex();
    rtl_li(s, s0, 0);
  } else {
    rtl_li(s, s0, 1);
  }
  rtl_sr(s, id_dest->reg, s0, 0);

  print_asm_template3(sc);
}

static void inline amo_load(DecodeExecState *s) {
  rtl_lm(s, s0, dsrc1, 0, s->width);
  rtl_sext(s, s0, s0, s->width);
}

static void inline amo_update(DecodeExecState *s) {
  rtl_sm(s, dsrc1, 0, s1, s->width);
  return_on_mem_ex();
  rtl_sr(s, id_dest->reg, s0, 0);
}

static inline def_EHelper(amoswap) {
  amo_load(s);
  return_on_mem_ex();
  rtl_mv(s, s1, dsrc2); // swap
  amo_update(s);
  return_on_mem_ex();
  print_asm_template3(amoswap);
}

static inline def_EHelper(amoadd) {
  amo_load(s);
  return_on_mem_ex();
  rtl_add(s, s1, s0, dsrc2);
  amo_update(s);
  return_on_mem_ex();
  print_asm_template3(amoor);
}

static inline def_EHelper(amoor) {
  amo_load(s);
  return_on_mem_ex();
  rtl_or(s, s1, s0, dsrc2);
  amo_update(s);
  return_on_mem_ex();
  print_asm_template3(amoor);
}

static inline def_EHelper(amoand) {
  amo_load(s);
  return_on_mem_ex();
  rtl_and(s, s1, s0, dsrc2);
  amo_update(s);
  return_on_mem_ex();
  print_asm_template3(amoand);
}

static inline def_EHelper(amomaxu) {
  amo_load(s);
  return_on_mem_ex();
  *s1 = (*s0 > *dsrc2 ? *s0 : *dsrc2);
  amo_update(s);
  return_on_mem_ex();
  print_asm_template3(amomaxu);
}

static inline def_EHelper(amomax) {
  amo_load(s);
  return_on_mem_ex();
  *s1 = (((sword_t)*s0) > ((sword_t)*dsrc2) ? *s0 : *dsrc2);
  amo_update(s);
  return_on_mem_ex();
  print_asm_template3(amomax);
}

static inline def_EHelper(amominu) {
  amo_load(s);
  return_on_mem_ex();
  *s1 = (*s0 < *dsrc2 ? *s0 : *dsrc2);
  amo_update(s);
  return_on_mem_ex();
  print_asm_template3(amominu);
}

static inline def_EHelper(amomin) {
  amo_load(s);
  return_on_mem_ex();
  *s1 = (((sword_t)*s0) < ((sword_t)*dsrc2) ? *s0 : *dsrc2);
  amo_update(s);
  return_on_mem_ex();
  print_asm_template3(amomin);
}

static inline def_EHelper(amoxor) {
  amo_load(s);
  return_on_mem_ex();
  rtl_xor(s, s1, s0, dsrc2);
  amo_update(s);
  return_on_mem_ex();
  print_asm_template3(amoxor);
}
