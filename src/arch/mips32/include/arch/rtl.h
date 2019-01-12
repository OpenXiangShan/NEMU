#ifndef __MIPS32_RTL_H__
#define __MIPS32_RTL_H__

#include "cpu/rtl.h"

static inline void rtl_lr(rtlreg_t* dest, int r, int width) {
  rtl_mv(dest, &reg_l(r));
}

static inline void rtl_sr(int r, const rtlreg_t *src1, int width) {
  rtl_mv(&reg_l(r), src1);
}

#endif
