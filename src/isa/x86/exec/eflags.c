#include "../local-include/rtl.h"
#include "../local-include/reg.h"

#define EFLAGS_BIT_CF 0
#define EFLAGS_BIT_ZF 6
#define EFLAGS_BIT_SF 7
#define EFLAGS_BIT_IF 9
#define EFLAGS_BIT_OF 11

#define _EFLAGS(f) f(OF) f(IF) f(SF) f(ZF) f(CF)
#define __f(flag) concat(EFLAGS_MASK_, flag) = 1 << concat(EFLAGS_BIT_, flag),
enum {
  MAP(_EFLAGS, __f)
#undef __f
#define __f(flag) | concat(EFLAGS_MASK_, flag)
  EFLAGS_MASK_ALL = 0 MAP(_EFLAGS, __f)
#undef __f
};

#define ENCODE(flag) \
  rtl_shli(s, t0, &cpu.flag, concat(EFLAGS_BIT_, flag)); \
  rtl_or(s, dest, dest, t0);

#define DECODE(flag) \
  rtl_shri(s, &cpu.flag, src, concat(EFLAGS_BIT_, flag)); \
  rtl_andi(s, &cpu.flag, &cpu.flag, 0x1);

void rtl_compute_eflags(DecodeExecState *s, rtlreg_t *dest) {
  rtl_mv(s, dest, rz);
  MAP(_EFLAGS, ENCODE)
  rtl_ori(s, dest, dest, 0x2);
}

void rtl_set_eflags(DecodeExecState *s, const rtlreg_t *src) {
  MAP(_EFLAGS, DECODE)
}
