#include "cpu/rtl.h"

#define ENCODE(flag) \
  rtl_shli(&t0, &cpu.flag, concat(EFLAGS_BIT_, flag)); \
  rtl_or(dest, dest, &t0);

#define DECODE(flag) \
  rtl_shri(&cpu.flag, src, concat(EFLAGS_BIT_, flag)); \
  rtl_andi(&cpu.flag, &cpu.flag, 0x1);

void rtl_compute_eflags(rtlreg_t *dest) {
  rtl_li(dest, 0);
  MAP(_EFLAGS, ENCODE)
  rtl_ori(dest, dest, 0x2);
}

void rtl_set_eflags(const rtlreg_t *src) {
  MAP(_EFLAGS, DECODE)
}
