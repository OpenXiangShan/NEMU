#include <cpu/exec.h>
#include "cc.h"

#ifdef LAZY_CC
static inline make_rtl(set_lazycc, const rtlreg_t *dest, const rtlreg_t *src1, const rtlreg_t *src2,
    uint32_t cc_op, uint32_t width) {
  rtl_mv(s, &cpu.cc_dest, dest);
  if (src1 != NULL) rtl_mv(s, &cpu.cc_src1, src1);
  if (src2 != NULL) rtl_mv(s, &cpu.cc_src2, src2);
  cpu.cc_op = cc_op;
  cpu.cc_width = width;
}

static const int cc2relop [] = {
  [CC_O]  = -1,        [CC_NO]  = -1,
  [CC_B]  = RELOP_LTU, [CC_NB]  = RELOP_GEU,
  [CC_E]  = RELOP_EQ,  [CC_NE]  = RELOP_NE,
  [CC_BE] = RELOP_LEU, [CC_NBE] = RELOP_GTU,
  [CC_S]  = -1,        [CC_NS]  = -1,
  [CC_P]  = -1,        [CC_NP]  = -1,
  [CC_L]  = RELOP_LT,  [CC_NL]  = RELOP_GE,
  [CC_LE] = RELOP_LE,  [CC_NLE] = RELOP_GT,
};

static inline make_rtl(lazy_jcc, uint32_t cc) {
  // common condition codes which only depend on cpu.cc_dest
  switch (cc) {
    case CC_E:  rtl_jrelop(s, RELOP_EQ, &cpu.cc_dest, rz, s->jmp_pc); return;
    case CC_NE: rtl_jrelop(s, RELOP_NE, &cpu.cc_dest, rz, s->jmp_pc); return;
    case CC_S:  rtl_jrelop(s, RELOP_LT, &cpu.cc_dest, rz, s->jmp_pc); return;
    case CC_NS: rtl_jrelop(s, RELOP_GE, &cpu.cc_dest, rz, s->jmp_pc); return;
  }

  switch (cpu.cc_op) {
    case LAZYCC_DEC:
      rtl_li(s, &cpu.cc_src2, 1);
      // fall through
    case LAZYCC_SBB: // FIXME: should consider CF
    case LAZYCC_SUB:
      if (cc2relop[cc] != -1) {
        //Log("cc = %d, src1 = 0x%x, src2 = 0x%x", cc, cpu.cc_src1, cpu.cc_src2);
        rtl_jrelop(s, cc2relop[cc], &cpu.cc_src1, &cpu.cc_src2, s->jmp_pc);
        return;
      }
      break;
    case LAZYCC_LOGIC:
      if (cc == CC_LE) {
        rtl_jrelop(s, cc2relop[cc], &cpu.cc_dest, rz, s->jmp_pc);
        return;
      }
      break;
    default: panic("unhandle cc_op = %d", cpu.cc_op);
  }

  panic("unhandle cc_op = %d, cc = %d", cpu.cc_op, cc);
}

static inline make_rtl(lazy_setcc, rtlreg_t *dest, uint32_t cc) {
  switch (cpu.cc_op) {
    case LAZYCC_SUB:
      if (cc2relop[cc] != -1) {
        rtl_setrelop(s, cc2relop[cc], dest, &cpu.cc_src1, &cpu.cc_src2);
        return;
      }
      break;
    case LAZYCC_LOGIC:
      if (cc == CC_E || cc == CC_NE || cc == CC_LE) {
        rtl_setrelop(s, cc2relop[cc], dest, &cpu.cc_dest, rz);
        return;
      }
      break;
    default: panic("unhandle cc_op = %d", cpu.cc_op);
  }
  panic("unhandle cc_op = %d, cc = %d", cpu.cc_op, cc);
}
#endif
