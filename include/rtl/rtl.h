#ifndef __RTL_RTL_H__
#define __RTL_RTL_H__

#include <cpu/decode.h>
#include <rtl/c_op.h>
#include <rtl/relop.h>

#define id_src1 (&s->src1)
#define id_src2 (&s->src2)
#define id_dest (&s->dest)

#define dsrc1 (&id_src1->val)
#define dsrc2 (&id_src2->val)
#define ddest (&id_dest->val)
#define ir    (&s->tmp_reg[0])
#define s0    (&s->tmp_reg[1])
#define s1    (&s->tmp_reg[2])
#define t0    (&s->tmp_reg[3])
#define t1    (&s->tmp_reg[4])

#define make_rtl(name, ...) void concat(rtl_, name)(DecodeExecState *s, __VA_ARGS__)

bool interpret_relop(uint32_t relop, const rtlreg_t src1, const rtlreg_t src2);

/* RTL basic instructions */

static inline make_rtl(li, rtlreg_t* dest, rtlreg_t imm) {
  *dest = imm;
}

static inline make_rtl(mv, rtlreg_t* dest, const rtlreg_t *src1) {
  *dest = *src1;
}

#define make_rtl_arith_logic(name) \
  static inline make_rtl(name, rtlreg_t* dest, const rtlreg_t* src1, const rtlreg_t* src2) { \
    *dest = concat(c_, name) (*src1, *src2); \
  } \
  /* Actually those of imm version are pseudo rtl instructions,
   * but we define them here in the same macro */ \
  static inline make_rtl(name ## i, rtlreg_t* dest, const rtlreg_t* src1, sword_t imm) { \
    rtl_li(s, ir, imm); \
    rtl_ ## name (s, dest, src1, ir); \
  }

make_rtl_arith_logic(add)
make_rtl_arith_logic(sub)
make_rtl_arith_logic(and)
make_rtl_arith_logic(or)
make_rtl_arith_logic(xor)
make_rtl_arith_logic(shl)
make_rtl_arith_logic(shr)
make_rtl_arith_logic(sar)
make_rtl_arith_logic(mul_lo)
make_rtl_arith_logic(mul_hi)
make_rtl_arith_logic(imul_lo)
make_rtl_arith_logic(imul_hi)
make_rtl_arith_logic(div_q)
make_rtl_arith_logic(div_r)
make_rtl_arith_logic(idiv_q)
make_rtl_arith_logic(idiv_r)

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

static inline make_rtl(lm, rtlreg_t *dest, const rtlreg_t* addr, int len) {
  *dest = vaddr_read(*addr, len);
}

static inline make_rtl(sm, const rtlreg_t* addr, const rtlreg_t* src1, int len) {
  vaddr_write(*addr, *src1, len);
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

static inline make_rtl(setrelop, uint32_t relop, rtlreg_t *dest,
    const rtlreg_t *src1, const rtlreg_t *src2) {
  *dest = interpret_relop(relop, *src1, *src2);
}

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

void rtl_exit(int state, vaddr_t halt_pc, uint32_t halt_ret);


/* RTL pseudo instructions */

static inline make_rtl(not, rtlreg_t *dest, const rtlreg_t* src1) {
  // dest <- ~src1
//  TODO();
  rtl_xori(s, dest, src1, -1);
}

static inline make_rtl(sext, rtlreg_t* dest, const rtlreg_t* src1, int width) {
  // dest <- signext(src1[(width * 8 - 1) .. 0])
//  TODO();

#ifdef ISA64
  if (width == 8) {
    rtl_mv(s, dest, src1);
  } else {
    assert(width == 1 || width == 2 || width == 4);
    rtl_shli(s, dest, src1, (8 - width) * 8);
    rtl_sari(s, dest, dest, (8 - width) * 8);
  }
#else
  if (width == 4) {
    rtl_mv(s, dest, src1);
  } else {
    assert(width == 1 || width == 2);
    rtl_shli(s, dest, src1, (4 - width) * 8);
    rtl_sari(s, dest, dest, (4 - width) * 8);
  }
#endif
}

static inline make_rtl(setrelopi, uint32_t relop, rtlreg_t *dest,
    const rtlreg_t *src1, int imm) {
  rtl_li(s, ir, imm);
  rtl_setrelop(s, relop, dest, src1, ir);
}

static inline make_rtl(msb, rtlreg_t* dest, const rtlreg_t* src1, int width) {
  // dest <- src1[width * 8 - 1]
//  TODO();
  rtl_shri(s, dest, src1, width * 8 - 1);
  if (width != 4) {
    rtl_andi(s, dest, dest, 0x1);
  }
}

static inline make_rtl(mux, rtlreg_t* dest, const rtlreg_t* cond, const rtlreg_t* src1, const rtlreg_t* src2) {
  // dest <- (cond ? src1 : src2)
//  TODO();
  rtl_setrelopi(s, RELOP_EQ, t0, cond, 0);
  rtl_subi(s, t0, t0, 1);
  // t0 = mask
  rtl_and(s, t1, src1, t0);
  rtl_not(s, t0, t0);
  rtl_and(s, t0, src2, t0);
  rtl_or(s, dest, t0, t1);
}

#endif
