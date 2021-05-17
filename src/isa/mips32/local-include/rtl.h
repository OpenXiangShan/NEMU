#ifndef __MIPS32_RTL_H__
#define __MIPS32_RTL_H__

#include <rtl/rtl.h>
#include "reg.h"

static inline def_rtl(mux, rtlreg_t* dest, const rtlreg_t* cond,
    const rtlreg_t* src1, const rtlreg_t* src2) {
  // dest <- (cond ? src1 : src2)
  rtl_setrelopi(s, RELOP_EQ, s0, cond, 0);
  rtl_subi(s, s0, s0, 1);
  // s0 = mask
  rtl_and(s, s1, src1, s0);
  rtl_not(s, s0, s0);
  rtl_and(s, dest, src2, s0);
  rtl_or(s, dest, dest, s1);
}

#endif
