#include <cpu/exec.h>
#include "cc.h"

#ifdef LAZY_CC
static inline make_rtl(set_lazycc_dest, const rtlreg_t *dest) {
  rtl_mv(s, &cpu.cc_dest, dest);
}

static inline make_rtl(set_lazycc_src1, const rtlreg_t *src1) {
  rtl_mv(s, &cpu.cc_src1, src1);
}

static inline make_rtl(set_lazycc_src2, const rtlreg_t *src2) {
  rtl_mv(s, &cpu.cc_src2, src2);
}

static inline make_rtl(set_lazycc, const rtlreg_t *dest, const rtlreg_t *src1, const rtlreg_t *src2,
    uint32_t cc_op, uint32_t width) {
  rtl_set_lazycc_dest(s, dest);
  if (src1 != NULL) rtl_set_lazycc_src1(s, src1);
  if (src2 != NULL) rtl_set_lazycc_src2(s, src2);
  cpu.cc_op = cc_op;
  cpu.cc_width = width;
}

#define UNARY 0x100  // compare with cpu.cc_dest and rz
static const int cc2relop [] = {
  [CC_O]  = 0,         [CC_NO]  = 0,
  [CC_B]  = RELOP_LTU, [CC_NB]  = RELOP_GEU,
  [CC_E]  = UNARY | RELOP_EQ,  [CC_NE]  = UNARY | RELOP_NE,
  [CC_BE] = RELOP_LEU, [CC_NBE] = RELOP_GTU,
  [CC_S]  = UNARY | RELOP_LT,  [CC_NS]  = UNARY | RELOP_GE,
  [CC_P]  = 0,         [CC_NP]  = 0,
  [CC_L]  = RELOP_LT,  [CC_NL]  = RELOP_GE,
  [CC_LE] = RELOP_LE,  [CC_NLE] = RELOP_GT,
};

static inline make_rtl(lazy_jcc, uint32_t cc) {
  int exception = (cpu.cc_op == LAZYCC_SUB) && (cc == CC_E || cc == CC_NE);
  if ((cc2relop[cc] & UNARY) && !exception) {
    uint32_t relop = cc2relop[cc] ^ UNARY;
    rtlreg_t *p = &cpu.cc_dest;
    if (cpu.cc_op == LAZYCC_SUB) {
      // sub && (CC_S || CC_NS)
      rtl_sub(s, s2, &cpu.cc_dest, &cpu.cc_src1);
      p = s2;
    }
    int exception = (cpu.cc_op == LAZYCC_LOGIC) && (cc == CC_E || cc == CC_NE);
    if (cpu.cc_width != 4 && !exception) {
      rtl_shli(s, s2, p, 32 - cpu.cc_width * 8);
      p = s2;
    }
    rtl_jrelop(s, relop, p, rz, s->jmp_pc);
    return;
  }

  switch (cpu.cc_op) {
    case LAZYCC_DEC:
      if (cc2relop[cc] != 0) {
        rtl_jrelop(s, cc2relop[cc], &cpu.cc_dest, rz, s->jmp_pc);
        return;
      }
      break;
    case LAZYCC_SBB: // FIXME: should consider CF
      if (cc == CC_B) {
        rtl_sub(s, s0, &cpu.cc_src1, &cpu.cc_dest);
        rtl_is_add_carry(s, s0, s0, &cpu.cc_src2);
        rtl_is_sub_carry(s, s1, &cpu.cc_src1, &cpu.cc_dest);
        rtl_or(s, s0, s0, s1);
        rtl_jrelop(s, RELOP_NE, s0, rz, s->jmp_pc);
        return;
      }
      break;
    case LAZYCC_SUB:
      if (cc2relop[cc] != 0) {
        rtl_jrelop(s, cc2relop[cc] & ~UNARY, &cpu.cc_dest, &cpu.cc_src1, s->jmp_pc);
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
  int exception = (cpu.cc_op == LAZYCC_SUB) && (cc == CC_E || cc == CC_NE);
  if ((cc2relop[cc] & UNARY) && !exception) {
    uint32_t relop = cc2relop[cc] ^ UNARY;
    rtlreg_t *p = &cpu.cc_dest;
    if (cpu.cc_op == LAZYCC_SUB) {
      // sub && (CC_S || CC_NS)
      rtl_sub(s, dest, &cpu.cc_dest, &cpu.cc_src1);
      p = dest;
    }
    int exception = (cpu.cc_op == LAZYCC_LOGIC) && (cc == CC_E || cc == CC_NE);
    if (cpu.cc_width != 4 && !exception) {
      rtl_shli(s, dest, p, 32 - cpu.cc_width * 8);
      p = dest;
    }
    rtl_setrelop(s, relop, dest, p, rz);
    return;
  }

  switch (cpu.cc_op) {
    case LAZYCC_ADD:
      if (cc2relop[cc] != 0) {
        rtlreg_t *p = &cpu.cc_dest;
        if (cpu.cc_width != 4) {
          rtl_andi(s, dest, &cpu.cc_dest, 0xffffffffu >> ((4 - cpu.cc_width) * 8));
          p = dest;
        }
        rtl_setrelop(s, cc2relop[cc], dest, p, &cpu.cc_src1);
        return;
      }
      if (cc == CC_O) {
        rtl_sub(s, dest, &cpu.cc_dest, &cpu.cc_src1);
        rtl_is_add_overflow(s, dest, &cpu.cc_dest, &cpu.cc_src1, dest, cpu.cc_width);
        return;
      }
      break;
    case LAZYCC_SUB:
      if (cc2relop[cc] != 0) {
        rtl_setrelop(s, cc2relop[cc] & ~UNARY, dest, &cpu.cc_dest, &cpu.cc_src1);
        return;
      }
      if (cc == CC_O) {
        rtl_sub(s, dest, &cpu.cc_dest, &cpu.cc_src1);
        rtl_is_sub_overflow(s, dest, dest, &cpu.cc_dest, &cpu.cc_src1, cpu.cc_width);
        return;
      }
      break;
    case LAZYCC_NEG:
      if (cc == CC_B) {
        rtl_setrelopi(s, RELOP_NE, dest, &cpu.cc_dest, 0);
        return;
      }
      if (cc == CC_O) {
        rtl_setrelopi(s, RELOP_EQ, dest, &cpu.cc_dest, -(0x1u << (cpu.cc_width * 8 - 1)));
        return;
      }
      break;
    case LAZYCC_INC:
      if (cc == CC_O) {
        rtl_setrelopi(s, RELOP_EQ, dest, &cpu.cc_dest, 0x1u << (cpu.cc_width * 8 - 1));
        return;
      }
      break;
    case LAZYCC_DEC:
      if (cc == CC_O) {
        rtl_addi(s, dest, &cpu.cc_dest, 1);
        rtl_setrelopi(s, RELOP_EQ, dest, dest, 0x1u << (cpu.cc_width * 8 - 1));
        return;
      }
      break;
    case LAZYCC_ADC:
      if (cc == CC_B) {
        rtlreg_t *p = &cpu.cc_dest;
        if (cpu.cc_width != 4) {
          rtl_andi(s, dest, &cpu.cc_dest, 0xffffffffu >> ((4 - cpu.cc_width) * 8));
          p = dest;
        }
        rtl_is_add_carry(s, t0, &cpu.cc_src1, &cpu.cc_src2);
        rtl_is_add_carry(s, dest, p, &cpu.cc_src1);
        rtl_or(s, dest, t0, dest);
        return;
      }
      if (cc == CC_O) {
        rtl_sub(s, dest, &cpu.cc_dest, &cpu.cc_src1);
        rtl_is_add_overflow(s, dest, &cpu.cc_dest, dest, &cpu.cc_src2, cpu.cc_width);
        return;
      }
      break;
    case LAZYCC_SBB:
      if (cc == CC_B) {
        rtl_sub(s, s0, &cpu.cc_src1, &cpu.cc_dest);
        rtl_is_add_carry(s, s0, s0, &cpu.cc_src2);
        rtl_is_sub_carry(s, s1, &cpu.cc_src1, &cpu.cc_dest);
        rtl_or(s, dest, s0, s1);
        return;
      }
      if (cc == CC_O) {
        rtl_is_sub_overflow(s, dest, &cpu.cc_dest, &cpu.cc_src1, &cpu.cc_src2, cpu.cc_width);
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
