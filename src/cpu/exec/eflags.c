#include "cpu/rtl.h"

void rtl_compute_eflags(rtlreg_t *dest) {
  rtl_li(dest, 0);

#define __f(X) \
  rtl_shli(&at, &cpu.X, concat(EFLAGS_BIT_, X)); \
  rtl_or(dest, dest, &at);

  __map_eflags(__f)

#undef __f

  rtl_ori(dest, dest, 0x2);
}

void rtl_set_eflags(const rtlreg_t *src) {
#define __f(X) \
  rtl_shri(&at, src, concat(EFLAGS_BIT_, X)); \
  rtl_andi(&cpu.X, &at, 0x1);

  __map_eflags(__f)

#undef __f
}
