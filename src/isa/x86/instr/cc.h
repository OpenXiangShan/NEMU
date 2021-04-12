#ifndef __CC_H__
#define __CC_H__

#include "../local-include/rtl.h"

enum {
  CC_O, CC_NO, CC_B,  CC_NB,
  CC_E, CC_NE, CC_BE, CC_NBE,
  CC_S, CC_NS, CC_P,  CC_NP,
  CC_L, CC_NL, CC_LE, CC_NLE
};

#ifndef __ICS_EXPORT
enum {
  LAZYCC_ADD,
  LAZYCC_SUB,
  LAZYCC_INC,
  LAZYCC_DEC,
  LAZYCC_NEG,
  LAZYCC_ADC,
  LAZYCC_SBB,
  LAZYCC_LOGIC,
};

#include "lazycc.h"
#endif

/* Condition Code */

static inline const char* get_cc_name(int subcode) {
  static const char *cc_name[] = {
    "o", "no", "b", "nb",
    "e", "ne", "be", "nbe",
    "s", "ns", "p", "np",
    "l", "nl", "le", "nle"
  };
  return cc_name[subcode];
}

static inline void rtl_setcc(Decode *s, rtlreg_t* dest, uint32_t subcode) {
  uint32_t invert = subcode & 0x1;

  // TODO: Query EFLAGS to determine whether the condition code is satisfied.
  // dest <- ( cc is satisfied ? 1 : 0)
  switch (subcode & 0xe) {
#ifdef __ICS_EXPORT
    case CC_O:
    case CC_B:
    case CC_E:
    case CC_BE:
    case CC_S:
    case CC_L:
    case CC_LE:
       TODO();
    case CC_P: panic("PF is not supported");
#else
    case CC_O: rtl_mv(s, dest, &cpu.OF); break;
    case CC_B: rtl_mv(s, dest, &cpu.CF); break;
    case CC_E: rtl_mv(s, dest, &cpu.ZF); break;
    case CC_BE: rtl_or(s, dest, &cpu.CF, &cpu.ZF); break;
    case CC_S: rtl_mv(s, dest, &cpu.SF); break;
    case CC_L: rtl_xor(s, dest, &cpu.SF, &cpu.OF); break;
    case CC_LE: rtl_xor(s, dest, &cpu.SF, &cpu.OF);
                rtl_or(s, dest, dest, &cpu.ZF);
                break;
    case CC_P: rtl_mv(s, dest, &cpu.PF); break;
#endif
    default: panic("should not reach here");
  }

  if (invert) {
    rtl_xori(s, dest, dest, 0x1);
  }
#ifdef __ICS_EXPORT
  assert(*dest == 0 || *dest == 1);
#else
#ifdef CONFIG_ENGINE_INTERPRETER
  // we can not do runtime checking in JIT for SDI
  assert(*dest == 0 || *dest == 1);
#endif
#endif
}

#endif
