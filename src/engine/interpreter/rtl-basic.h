#ifndef __RTL_BASIC_H__
#define __RTL_BASIC_H__

#include "c_op.h"
#include <memory/vaddr.h>

/* RTL basic instructions */

#define make_rtl_compute_reg(name) \
  static inline make_rtl(name, rtlreg_t* dest, const rtlreg_t* src1, const rtlreg_t* src2) { \
    *dest = concat(c_, name) (*src1, *src2); \
  }

#define make_rtl_compute_imm(name) \
  static inline make_rtl(name ## i, rtlreg_t* dest, const rtlreg_t* src1, const sword_t imm) { \
    *dest = concat(c_, name) (*src1, imm); \
  }

#define make_rtl_compute_reg_imm(name) \
  make_rtl_compute_reg(name) \
  make_rtl_compute_imm(name) \

// compute

make_rtl_compute_reg_imm(add)
make_rtl_compute_reg_imm(sub)
make_rtl_compute_reg_imm(and)
make_rtl_compute_reg_imm(or)
make_rtl_compute_reg_imm(xor)
make_rtl_compute_reg_imm(shl)
make_rtl_compute_reg_imm(shr)
make_rtl_compute_reg_imm(sar)

static inline make_rtl(setrelop, uint32_t relop, rtlreg_t *dest,
    const rtlreg_t *src1, const rtlreg_t *src2) {
  *dest = interpret_relop(relop, *src1, *src2);
}

static inline make_rtl(setrelopi, uint32_t relop, rtlreg_t *dest,
    const rtlreg_t *src1, sword_t imm) {
  *dest = interpret_relop(relop, *src1, imm);
}

// mul/div

make_rtl_compute_reg(mul_lo)
make_rtl_compute_reg(mul_hi)
make_rtl_compute_reg(imul_lo)
make_rtl_compute_reg(imul_hi)
make_rtl_compute_reg(div_q)
make_rtl_compute_reg(div_r)
make_rtl_compute_reg(idiv_q)
make_rtl_compute_reg(idiv_r)

static inline make_rtl(div64_q, rtlreg_t* dest,
    const rtlreg_t* src1_hi, const rtlreg_t* src1_lo, const rtlreg_t* src2) {
  uint64_t dividend = ((uint64_t)(*src1_hi) << 32) | (*src1_lo);
  uint32_t divisor = (*src2);
  *dest = dividend / divisor;
}

static inline make_rtl(div64_r, rtlreg_t* dest,
    const rtlreg_t* src1_hi, const rtlreg_t* src1_lo, const rtlreg_t* src2) {
  uint64_t dividend = ((uint64_t)(*src1_hi) << 32) | (*src1_lo);
  uint32_t divisor = (*src2);
  *dest = dividend % divisor;
}

static inline make_rtl(idiv64_q, rtlreg_t* dest,
    const rtlreg_t* src1_hi, const rtlreg_t* src1_lo, const rtlreg_t* src2) {
  int64_t dividend = ((uint64_t)(*src1_hi) << 32) | (*src1_lo);
  int32_t divisor = (*src2);
  *dest = dividend / divisor;
}

static inline make_rtl(idiv64_r, rtlreg_t* dest,
    const rtlreg_t* src1_hi, const rtlreg_t* src1_lo, const rtlreg_t* src2) {
  int64_t dividend = ((uint64_t)(*src1_hi) << 32) | (*src1_lo);
  int32_t divisor = (*src2);
  *dest = dividend % divisor;
}

// memory

static inline make_rtl(lm, rtlreg_t *dest, const rtlreg_t* addr, word_t offset, int len) {
  *dest = vaddr_read(*addr + offset, len);
}

static inline make_rtl(sm, const rtlreg_t* addr, word_t offset, const rtlreg_t* src1, int len) {
  vaddr_write(*addr + offset, *src1, len);
}

static inline make_rtl(host_lm, rtlreg_t* dest, const void *addr, int len) {
  switch (len) {
    case 4: *dest = *(uint32_t *)addr; return;
    case 1: *dest = *( uint8_t *)addr; return;
    case 2: *dest = *(uint16_t *)addr; return;
    default: assert(0);
  }
}

static inline make_rtl(host_sm, void *addr, const rtlreg_t *src1, int len) {
  switch (len) {
    case 4: *(uint32_t *)addr = *src1; return;
    case 1: *( uint8_t *)addr = *src1; return;
    case 2: *(uint16_t *)addr = *src1; return;
    default: assert(0);
  }
}

// control

static inline make_rtl(j, vaddr_t target) {
  s->jmp_pc = target;
  s->is_jmp = true;
}

static inline make_rtl(jr, rtlreg_t *target) {
  s->jmp_pc = *target;
  s->is_jmp = true;
}

static inline make_rtl(jrelop, uint32_t relop,
    const rtlreg_t *src1, const rtlreg_t *src2, vaddr_t target) {
  bool is_jmp = interpret_relop(relop, *src1, *src2);
  if (is_jmp) rtl_j(s, target);
}

/* Pseudo instructions */

static inline make_rtl(kill, const rtlreg_t *src1) {
  // do nothing
}

#endif
