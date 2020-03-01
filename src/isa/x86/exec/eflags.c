#include "rtl/rtl.h"

#define ENCODE(flag) \
  rtl_shli(s, t0, &cpu.flag, concat(EFLAGS_BIT_, flag)); \
  rtl_or(s, dest, dest, t0);

#define DECODE(flag) \
  rtl_shri(s, &cpu.flag, src, concat(EFLAGS_BIT_, flag)); \
  rtl_andi(s, &cpu.flag, &cpu.flag, 0x1);

void rtl_compute_eflags(DecodeExecState *s, rtlreg_t *dest) {
  rtl_li(s, dest, 0);
  MAP(_EFLAGS, ENCODE)
  rtl_ori(s, dest, dest, 0x2);
}

void rtl_set_eflags(DecodeExecState *s, const rtlreg_t *src) {
  MAP(_EFLAGS, DECODE)
}
