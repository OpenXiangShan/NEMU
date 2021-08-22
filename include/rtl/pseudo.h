#ifndef __RTL_PSEUDO_H__
#define __RTL_PSEUDO_H__

#ifndef __RTL_RTL_H__
#error "Should be only included by <rtl/rtl.h>"
#endif

/* RTL pseudo instructions */

static inline def_rtl(li, rtlreg_t* dest, const rtlreg_t imm) {
  rtl_addi(s, dest, rz, imm);
}

static inline def_rtl(mv, rtlreg_t* dest, const rtlreg_t *src1) {
  rtl_addi(s, dest, src1, 0);
}

static inline def_rtl(not, rtlreg_t *dest, const rtlreg_t* src1) {
  // dest <- ~src1
#ifdef __ICS_EXPORT
  TODO();
#else
  rtl_xori(s, dest, src1, -1);
#endif
}

static inline def_rtl(neg, rtlreg_t *dest, const rtlreg_t* src1) {
  // dest <- -src1
#ifdef __ICS_EXPORT
  TODO();
#else
  rtl_sub(s, dest, rz, src1);
#endif
}

static inline def_rtl(sext, rtlreg_t* dest, const rtlreg_t* src1, int width) {
  // dest <- signext(src1[(width * 8 - 1) .. 0])
#ifdef __ICS_EXPORT
  TODO();
#else
  const int word_size = sizeof(word_t);
  if (width == word_size) {
    rtl_mv(s, dest, src1);
  } else {
#ifdef CONFIG_RT_CHECK
    assert(width == 1 || width == 2 || MUXDEF(CONFIG_ISA64, width == 4, false));
#endif
    rtl_slli(s, dest, src1, (word_size - width) * 8);
    rtl_srai(s, dest, dest, (word_size - width) * 8);
  }
#endif
}

static inline def_rtl(zext, rtlreg_t* dest, const rtlreg_t* src1, int width) {
  // dest <- zeroext(src1[(width * 8 - 1) .. 0])
#ifdef __ICS_EXPORT
  TODO();
#else
  const int word_size = sizeof(word_t);
  if (width == word_size) {
    rtl_mv(s, dest, src1);
  } else {
#ifdef CONFIG_RT_CHECK
    assert(width == 1 || width == 2 || MUXDEF(CONFIG_ISA64, width == 4, false));
#endif
    rtl_slli(s, dest, src1, (word_size - width) * 8);
    rtl_srli(s, dest, dest, (word_size - width) * 8);
  }
#endif
}

static inline def_rtl(msb, rtlreg_t* dest, const rtlreg_t* src1, int width) {
  // dest <- src1[width * 8 - 1]
#ifdef __ICS_EXPORT
  TODO();
#else
  rtl_srli(s, dest, src1, width * 8 - 1);
  if (width != 4) {
    rtl_andi(s, dest, dest, 0x1);
  }
#endif
}
#ifndef __ICS_EXPORT

static inline def_rtl(trap, vaddr_t ret_pc, word_t NO) {
  rtl_li(s, t0, ret_pc);
  rtl_hostcall(s, HOSTCALL_TRAP, t0, t0, NULL, NO);
  rtl_jr(s, t0);
}
#endif
#endif
