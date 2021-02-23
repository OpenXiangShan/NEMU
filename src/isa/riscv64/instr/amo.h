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

#define amo_body(name, body) { \
  rtl_lm(s, s0, dsrc1, 0, s->width); \
  return_on_mem_ex(); \
  rtl_sext(s, s0, s0, s->width); \
  body; \
  rtl_sm(s, dsrc1, 0, s1, s->width); \
  return_on_mem_ex(); \
  rtl_sr(s, id_dest->reg, s0, 0); \
  print_asm_template3(concat(amo, name)); \
}

static inline def_EHelper(amoswap) {
  amo_body(swap, {
      rtl_mv(s, s1, dsrc2); // swap
  });
}

static inline def_EHelper(amoadd) {
  amo_body(add, {
      rtl_add(s, s1, s0, dsrc2);
  });
}

static inline def_EHelper(amoor) {
  amo_body(or, {
      rtl_or(s, s1, s0, dsrc2);
  });
}

static inline def_EHelper(amoand) {
  amo_body(and, {
      rtl_and(s, s1, s0, dsrc2);
  });
}

static inline def_EHelper(amomaxu) {
  amo_body(maxu, {
      *s1 = (*s0 > *dsrc2 ? *s0 : *dsrc2);
  });
}

static inline def_EHelper(amomax) {
  amo_body(max, {
      *s1 = (((sword_t)*s0) > ((sword_t)*dsrc2) ? *s0 : *dsrc2);
  });
}

static inline def_EHelper(amominu) {
  amo_body(minu, {
      *s1 = (*s0 < *dsrc2 ? *s0 : *dsrc2);
  });
}

static inline def_EHelper(amomin) {
  amo_body(min, {
      *s1 = (((sword_t)*s0) < ((sword_t)*dsrc2) ? *s0 : *dsrc2);
  });
}

static inline def_EHelper(amoxor) {
  amo_body(xor, {
      rtl_xor(s, s1, s0, dsrc2);
  });
}
