#include "cpu/rtl.h"

/* Condition Code */

void rtl_setcc(rtlreg_t* dest, uint8_t subcode) {
  bool invert = subcode & 0x1;
  enum {
    CC_O, CC_NO, CC_B,  CC_NB,
    CC_E, CC_NE, CC_BE, CC_NBE,
    CC_S, CC_NS, CC_P,  CC_NP,
    CC_L, CC_NL, CC_LE, CC_NLE
  };

  // TODO: Query EFLAGS to determine whether the condition code is satisfied.
  // dest <- ( cc is satisfied ? 1 : 0)
  switch (subcode & 0xe) {
    case CC_O: rtl_mv(dest, &cpu.OF); break;
    case CC_B: rtl_mv(dest, &cpu.CF); break;
    case CC_E: rtl_mv(dest, &cpu.ZF); break;
    case CC_BE: rtl_or(dest, &cpu.CF, &cpu.ZF); break;
    case CC_S: rtl_mv(dest, &cpu.SF); break;
    case CC_L: rtl_xor(dest, &cpu.SF, &cpu.OF); break;
    case CC_LE: rtl_xor(dest, &cpu.SF, &cpu.OF);
                rtl_or(dest, dest, &cpu.ZF);
                break;
                //TODO();
    default: panic("should not reach here");
    case CC_P: panic("n86 does not have PF");
  }

  if (invert) {
    rtl_xori(dest, dest, 0x1);
  }
  assert(*dest == 0 || *dest == 1);
}
