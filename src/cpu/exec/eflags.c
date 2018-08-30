#include "cpu/rtl.h"

void rtl_compute_eflags(rtlreg_t *dest) {
  rtl_li(dest, 0);
  rtl_shli(&at, &cpu.OF, 11);
  rtl_or(dest, dest, &at);

  rtl_shli(&at, &cpu.IF, 9);
  rtl_or(dest, dest, &at);

  rtl_shli(&at, &cpu.SF, 7);
  rtl_or(dest, dest, &at);

  rtl_shli(&at, &cpu.ZF, 6);
  rtl_or(dest, dest, &at);

  rtl_shli(&at, &cpu.CF, 0);
  rtl_or(dest, dest, &at);

  rtl_ori(dest, dest, 0x2);
}

void rtl_set_eflags(const rtlreg_t *src) {
  rtl_shri(&at, src, 11);
  rtl_andi(&cpu.OF, &at, 0x1);

  rtl_shri(&at, src, 9);
  rtl_andi(&cpu.IF, &at, 0x1);

  rtl_shri(&at, src, 7);
  rtl_andi(&cpu.SF, &at, 0x1);

  rtl_shri(&at, src, 6);
  rtl_andi(&cpu.ZF, &at, 0x1);

  rtl_shri(&at, src, 0);
  rtl_andi(&cpu.CF, &at, 0x1);
}
