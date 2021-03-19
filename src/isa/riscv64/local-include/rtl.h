#ifndef __RISCV64_RTL_H__
#define __RISCV64_RTL_H__

#include <rtl/rtl.h>
#include "reg.h"
#include "csr.h"

#define FBOX_MASK 0xFFFFFFFF00000000ull

static inline def_rtl(fbox, rtlreg_t *dest, rtlreg_t *src) {
  rtl_ori(s, dest, src, FBOX_MASK);
}

static inline def_rtl(funbox, rtlreg_t *dest, rtlreg_t *src) {
  assert((*src & FBOX_MASK) == FBOX_MASK); // return defaultNaNF32UI;
  rtl_andi(s, dest, src, ~FBOX_MASK);
}

static inline def_rtl(fsr, rtlreg_t *fdest, rtlreg_t *src, int width) {
  if (width == 4) rtl_fbox(s, fdest, src);
  else if (width == 8) rtl_mv(s, fdest, src);
  else assert(0);
  void fp_set_dirty();
  fp_set_dirty();
}

#endif
