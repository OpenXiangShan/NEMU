#ifndef __RTL_PSEUDO_H__
#define __RTL_PSEUDO_H__

#ifndef __RTL_RTL_H__
#error "Should be only included by <rtl/rtl.h>"
#endif

/* RTL pseudo instructions */

static inline make_rtl(li, rtlreg_t* dest, const rtlreg_t imm) {
  rtl_addi(s, dest, rz, imm);
}

static inline make_rtl(mv, rtlreg_t* dest, const rtlreg_t *src1) {
  if (dest != src1) rtl_add(s, dest, src1, rz);
}

static inline make_rtl(not, rtlreg_t *dest, const rtlreg_t* src1) {
  // dest <- ~src1
//  TODO();
  rtl_xori(s, dest, src1, -1);
}

static inline make_rtl(sext, rtlreg_t* dest, const rtlreg_t* src1, int width) {
  // dest <- signext(src1[(width * 8 - 1) .. 0])
//  TODO();

  const int word_size = sizeof(word_t);
  if (width == word_size) {
    rtl_mv(s, dest, src1);
  } else {
    assert(width == 1 || width == 2 || width == 4);
    rtl_shli(s, dest, src1, (word_size - width) * 8);
    rtl_sari(s, dest, dest, (word_size - width) * 8);
  }
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
