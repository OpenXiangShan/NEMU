#include <rtl/rtl.h>

__attribute__((cold))
def_rtl(amo_slow_path, rtlreg_t *dest, const rtlreg_t *src1, const rtlreg_t *src2) {
  uint32_t funct5 = s->isa.instr.r.funct7 >> 2;
  int width = s->isa.instr.r.funct3 & 1 ? 8 : 4;

  if (funct5 == 0b00010) { // lr
    rtl_lms(s, dest, src1, 0, width);
    cpu.lr_addr = *src1;
    return;
  } else if (funct5 == 0b00011) { // sc
    // should check overlapping instead of equality
    int success = cpu.lr_addr == *src1;
    if (success) rtl_sm(s, src1, 0, src2, width);
    rtl_li(s, dest, !success);
    return;
  }

  cpu.amo = true;
  rtl_lms(s, s0, src1, 0, width);
  switch (funct5) {
    case 0b00001: rtl_mv (s, s1, src2); break;
    case 0b00000: rtl_add(s, s1, s0, src2); break;
    case 0b01000: rtl_or (s, s1, s0, src2); break;
    case 0b01100: rtl_and(s, s1, s0, src2); break;
    case 0b00100: rtl_xor(s, s1, s0, src2); break;
    case 0b11100: *s1 = (*s0 > *src2 ? *s0 : *src2); break;
    case 0b10100: *s1 = ((sword_t)*s0 > (sword_t)*src2 ? *s0 : *src2); break;
    case 0b11000: *s1 = (*s0 < *src2 ? *s0 : *src2); break;
    case 0b10000: *s1 = ((sword_t)*s0 < (sword_t)*src2 ? *s0 : *src2); break;
    default: assert(0);
  }
  rtl_sm(s, src1, 0, s1, width);
  rtl_mv(s, dest, s0);
  cpu.amo = false;
}
