#ifndef __LAZYCC_H__
#define __LAZYCC_H__
#include <rtl/rtl.h>
#include "cc.h"

#ifdef CONFIG_x86_CC_LAZY
enum {
  LAZYCC_ADD, LAZYCC_SUB, LAZYCC_INC, LAZYCC_DEC,
  LAZYCC_NEG, LAZYCC_ADC, LAZYCC_SBB, LAZYCC_LOGIC,
  LAZYCC_SHL, LAZYCC_SHR, LAZYCC_SAR, LAZYCC_BT,
  LAZYCC_MUL, LAZYCC_POPF, LAZYCC_FCMP, LAZYCC_FCMP_SAME,
};

static inline def_rtl(set_lazycc, const rtlreg_t *dest, const rtlreg_t *src1, const rtlreg_t *src2,
    uint32_t cc_op, uint32_t width) {
  if (dest != NULL) rtl_mv(s, &cpu.cc_dest, dest);
  if (src1 != NULL) rtl_mv(s, &cpu.cc_src1, src1);
  if (src2 != NULL) rtl_mv(s, &cpu.cc_src2, src2);
  cpu.cc_op = cc_op;
  cpu.cc_width = width;
  cpu.cc_dirty = true;
}

static inline void clean_lazycc() {
  cpu.cc_dirty = false;
}

enum { CCTYPE_SETCC, CCTYPE_JCC };

typedef struct {
  union {
    rtlreg_t *dest;
    word_t target;
  };
  const rtlreg_t *src1, *src2;
  int type;
  int relop;
  int invert;
} CCop;

def_rtl(lazy_setcc, rtlreg_t *dest, uint32_t cc);
def_rtl(lazy_jcc, CCop *op, uint32_t cc);

#endif
#endif
