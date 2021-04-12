#include <cpu/exec.h>
#include "cc.h"

#ifdef CONFIG_x86_CC_LAZY
static inline def_rtl(set_lazycc_dest, const rtlreg_t *dest) {
  rtl_mv(s, &cpu.cc_dest, dest);
}

static inline def_rtl(set_lazycc_src1, const rtlreg_t *src1) {
  rtl_mv(s, &cpu.cc_src1, src1);
}

static inline def_rtl(set_lazycc_src2, const rtlreg_t *src2) {
  rtl_mv(s, &cpu.cc_src2, src2);
}

static inline def_rtl(set_lazycc, const rtlreg_t *dest, const rtlreg_t *src1, const rtlreg_t *src2,
    uint32_t cc_op, uint32_t width) {
  rtl_set_lazycc_dest(s, dest);
  if (src1 != NULL) rtl_set_lazycc_src1(s, src1);
  if (src2 != NULL) rtl_set_lazycc_src2(s, src2);
  cpu.cc_op = cc_op;
  cpu.cc_width = width;
  cpu.cc_dirty = true;
}

static inline void clean_lazycc() {
  cpu.cc_dirty = false;
}

#define NEGCC(cc) ((cc)%2 == 1)
#define NEGCCRELOP(cc) (NEGCC(cc) ? RELOP_NE : RELOP_EQ)
#define MASKDEST(reg) \
  p = &cpu.cc_dest; \
  if (cpu.cc_width != 4) { rtl_andi(s, reg, &cpu.cc_dest, 0xffffffffu >> ((4 - cpu.cc_width) * 8)); p = reg;}

#define UNARY 0x100  // compare with cpu.cc_dest and rz
static const int cc2relop [] = {
  [CC_O]  = 0,                 [CC_NO]  = 0,
  [CC_B]  = RELOP_LTU,         [CC_NB]  = RELOP_GEU,
  [CC_E]  = UNARY | RELOP_EQ,  [CC_NE]  = UNARY | RELOP_NE,
  [CC_BE] = RELOP_LEU,         [CC_NBE] = RELOP_GTU,
  [CC_S]  = UNARY | RELOP_LT,  [CC_NS]  = UNARY | RELOP_GE,
  [CC_P]  = 0,                 [CC_NP]  = 0,
  [CC_L]  = RELOP_LT,          [CC_NL]  = RELOP_GE,
  [CC_LE] = RELOP_LE,          [CC_NLE] = RELOP_GT,
};

static const int cc2relop_logic [] = {
  [CC_O]  = RELOP_FALSE,       [CC_NO]  = RELOP_TRUE,
  [CC_B]  = RELOP_LTU,         [CC_NB]  = RELOP_GEU,
  [CC_E]  = RELOP_EQ,          [CC_NE]  = RELOP_NE,
  [CC_BE] = RELOP_LEU,         [CC_NBE] = RELOP_GTU,
  [CC_S]  = RELOP_LT,          [CC_NS]  = RELOP_GE,
  [CC_P]  = 0,                 [CC_NP]  = 0,
  [CC_L]  = RELOP_LT,          [CC_NL]  = RELOP_GE,
  [CC_LE] = RELOP_LE,          [CC_NLE] = RELOP_GT,
};


static inline def_rtl(lazy_setcc_internal, rtlreg_t *dest, uint32_t cc) {
  rtlreg_t *p = NULL;
  int exception = (cpu.cc_op == LAZYCC_SUB) && (cc == CC_E || cc == CC_NE);
  if ((cc2relop[cc] & UNARY) && !exception) {
    uint32_t relop = cc2relop[cc] ^ UNARY;
    p = &cpu.cc_dest;
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
      switch (cc) {
        case CC_O: case CC_NO:
          rtl_sub(s, dest, &cpu.cc_dest, &cpu.cc_src1);
          rtl_is_add_overflow(s, dest, &cpu.cc_dest, &cpu.cc_src1, dest, cpu.cc_width);
          goto negcc_reverse;
          return;
        case CC_LE: case CC_NLE:
          rtl_sub(s, dest, &cpu.cc_dest, &cpu.cc_src1);
          rtl_is_add_overflow(s, s0, &cpu.cc_dest, &cpu.cc_src1, dest, cpu.cc_width);
          rtl_msb(s, s1, &cpu.cc_dest, cpu.cc_width);
          rtl_xor(s, dest, s0, s1);
          MASKDEST(s0);
          rtl_setrelopi(s, RELOP_EQ, s0, p, 0);
          rtl_or(s, dest, dest, s0);
          goto negcc_reverse;
          return;
        case CC_BE: case CC_NBE:
          MASKDEST(s0);
          rtl_is_add_carry(s, s1, p, &cpu.cc_src1);
          rtl_setrelopi(s, RELOP_EQ, s0, p, 0);
          rtl_or(s, dest, s0, s1);
          goto negcc_reverse;
          return;
        default:
          if (cc2relop[cc] != 0) {
            MASKDEST(s0);
            rtl_setrelop(s, cc2relop[cc], dest, p, &cpu.cc_src1);
            return;
          }
      }
      break;
    case LAZYCC_SUB:
      switch (cc) {
        case CC_O: case CC_NO:
          rtl_sub(s, dest, &cpu.cc_dest, &cpu.cc_src1);
          rtl_is_sub_overflow(s, dest, dest, &cpu.cc_dest, &cpu.cc_src1, cpu.cc_width);
          goto negcc_reverse;
          return;
        default:
          if (cc2relop[cc] != 0) {
            rtl_setrelop(s, cc2relop[cc] & 0xf, dest, &cpu.cc_dest, &cpu.cc_src1);
            return;
          }
      }
      break;
    case LAZYCC_NEG:
      switch (cc) {
        case CC_B: case CC_NB:
          rtl_setrelopi(s, RELOP_NE, dest, &cpu.cc_dest, 0);
          goto negcc_reverse;
          return;
        case CC_O: case CC_NO:
          rtl_setrelopi(s, NEGCCRELOP(cc), dest, &cpu.cc_dest, -(0x1u << (cpu.cc_width * 8 - 1)));
          return;
        case CC_L: case CC_NL:
          rtl_setrelopi(s, NEGCCRELOP(cc), s0, &cpu.cc_dest, -(0x1u << (cpu.cc_width * 8 - 1)));
          rtl_msb(s, s1, &cpu.cc_dest, cpu.cc_width);
          rtl_xor(s, dest, s0, s1);
          return;
        case CC_LE: case CC_NLE:
          rtl_setrelopi(s, RELOP_EQ, s0, &cpu.cc_dest, -(0x1u << (cpu.cc_width * 8 - 1)));
          rtl_msb(s, s1, &cpu.cc_dest, cpu.cc_width);
          rtl_xor(s, dest, s0, s1);
          rtl_setrelopi(s, RELOP_EQ, s0, &cpu.cc_dest, 0);
          rtl_or(s, dest, dest, s0);
          goto negcc_reverse;
          return;
        case CC_BE:
          rtl_li(s, dest, 1);
          return;
        case CC_NBE:
          rtl_li(s, dest, 0);
          return;
      }
      break;
    case LAZYCC_INC:
      switch (cc) {
        case CC_O: case CC_NO:
          rtl_setrelopi(s, NEGCCRELOP(cc), dest, &cpu.cc_dest, 0x1u << (cpu.cc_width * 8 - 1));
          return;
        case CC_L: case CC_NL:
          rtl_setrelopi(s, NEGCCRELOP(cc), s0, &cpu.cc_dest, 0x1u << (cpu.cc_width * 8 - 1));
          rtl_msb(s, s1, &cpu.cc_dest, cpu.cc_width);
          rtl_xor(s, dest, s0, s1);
          return;
        case CC_LE: case CC_NLE:
          rtl_setrelopi(s, RELOP_EQ, s0, &cpu.cc_dest, 0x1u << (cpu.cc_width * 8 - 1));
          rtl_msb(s, s1, &cpu.cc_dest, cpu.cc_width);
          rtl_xor(s, dest, s0, s1);
          MASKDEST(s0);
          rtl_setrelopi(s, RELOP_EQ, s0, p, 0);
          rtl_or(s, dest, dest, s0);
          goto negcc_reverse;
          return;
      }
      break;
    case LAZYCC_DEC:
      switch (cc) {
        case CC_O: case CC_NO:
          rtl_addi(s, dest, &cpu.cc_dest, 1);
          rtl_setrelopi(s, NEGCCRELOP(cc), dest, dest, 0x1u << (cpu.cc_width * 8 - 1));
          return;
        case CC_L: case CC_NL:
          rtl_addi(s, s0, &cpu.cc_dest, 1);
          rtl_setrelopi(s, NEGCCRELOP(cc), s0, s0, 0x1u << (cpu.cc_width * 8 - 1));
          rtl_msb(s, s1, &cpu.cc_dest, cpu.cc_width);
          rtl_xor(s, dest, s0, s1);
          return;
        case CC_LE: case CC_NLE:
          rtl_addi(s, s0, &cpu.cc_dest, 1);
          rtl_setrelopi(s, RELOP_EQ, s0, s0, 0x1u << (cpu.cc_width * 8 - 1));
          rtl_msb(s, s1, &cpu.cc_dest, cpu.cc_width);
          rtl_xor(s, dest, s0, s1);
          rtl_setrelopi(s, RELOP_EQ, s0, &cpu.cc_dest, 0);
          rtl_or(s, dest, dest, s0);
          goto negcc_reverse;
          return;
      }
      break;
    case LAZYCC_ADC:
      switch (cc) {
        case CC_B: case CC_NB:
          MASKDEST(s0);
          rtl_is_add_carry(s, s1, &cpu.cc_src1, &cpu.cc_src2);
          rtl_is_add_carry(s, dest, p, &cpu.cc_src1);
          rtl_or(s, dest, s1, dest);
          goto negcc_reverse;
          return;
        case CC_O: case CC_NO:
          rtl_sub(s, dest, &cpu.cc_dest, &cpu.cc_src1);
          rtl_is_add_overflow(s, dest, &cpu.cc_dest, dest, &cpu.cc_src2, cpu.cc_width);
          goto negcc_reverse;
          return;
        case CC_L: case CC_NL:
          rtl_sub(s, dest, &cpu.cc_dest, &cpu.cc_src1);
          rtl_is_add_overflow(s, s0, &cpu.cc_dest, dest, &cpu.cc_src2, cpu.cc_width);
          rtl_msb(s, s1, &cpu.cc_dest, cpu.cc_width);
          rtl_xor(s, dest, s0, s1);
          goto negcc_reverse;
          return;
        case CC_LE: case CC_NLE:
          rtl_sub(s, dest, &cpu.cc_dest, &cpu.cc_src1);
          rtl_is_add_overflow(s, s0, &cpu.cc_dest, dest, &cpu.cc_src2, cpu.cc_width);
          rtl_msb(s, s1, &cpu.cc_dest, cpu.cc_width);
          rtl_xor(s, dest, s0, s1);
          MASKDEST(s0);
          rtl_setrelopi(s, RELOP_EQ, s0, p, 0);
          rtl_or(s, dest, dest, s0);
          goto negcc_reverse;
          return;
        case CC_BE: case CC_NBE:
          MASKDEST(s0);
          rtl_setrelopi(s, RELOP_EQ, s1, p, 0);
          rtl_is_add_carry(s, t0, &cpu.cc_src1, &cpu.cc_src2);
          rtl_is_add_carry(s, s0, p, &cpu.cc_src1);
          rtl_or(s, dest, t0, s0);
          rtl_or(s, dest, dest, s1);
          goto negcc_reverse;
          return;
      }
      break;
    case LAZYCC_SBB:
      switch (cc) {
        case CC_B: case CC_NB:
          rtl_sub(s, s0, &cpu.cc_src1, &cpu.cc_dest);
          rtl_is_add_carry(s, s0, s0, &cpu.cc_src2);
          rtl_is_sub_carry(s, s1, &cpu.cc_src1, &cpu.cc_dest);
          rtl_or(s, dest, s0, s1);
          goto negcc_reverse;
          return;
        case CC_O: case CC_NO:
          rtl_is_sub_overflow(s, dest, &cpu.cc_dest, &cpu.cc_src1, &cpu.cc_src2, cpu.cc_width);
          goto negcc_reverse;
          return;
        case CC_L: case CC_NL:
          rtl_is_sub_overflow(s, s0, &cpu.cc_dest, &cpu.cc_src1, &cpu.cc_src2, cpu.cc_width);
          rtl_msb(s, s1, &cpu.cc_dest, cpu.cc_width);
          rtl_xor(s, dest, s0, s1);
          goto negcc_reverse;
          return;
        case CC_LE: case CC_NLE:
          rtl_is_sub_overflow(s, s0, &cpu.cc_dest, &cpu.cc_src1, &cpu.cc_src2, cpu.cc_width);
          rtl_msb(s, s1, &cpu.cc_dest, cpu.cc_width);
          rtl_xor(s, dest, s0, s1);
          MASKDEST(s0);
          rtl_setrelopi(s, RELOP_EQ, s0, p, 0);
          rtl_or(s, dest, dest, s0);
          goto negcc_reverse;
          return;
        case CC_BE: case CC_NBE:
          rtl_sub(s, s0, &cpu.cc_src1, &cpu.cc_dest);
          rtl_is_add_carry(s, s0, s0, &cpu.cc_src2);
          rtl_is_sub_carry(s, s1, &cpu.cc_src1, &cpu.cc_dest);
          rtl_or(s, dest, s0, s1);
          rtl_setrelopi(s, RELOP_EQ, s0, &cpu.cc_dest, 0);
          rtl_or(s, dest, dest, s0);
          goto negcc_reverse;
          return;
      }
      break;
    case LAZYCC_LOGIC:
      switch (cc) {
        case CC_LE: case CC_NLE: case CC_L: case CC_NL:
          p = &cpu.cc_dest;
          if (cpu.cc_width != 4) {
            rtl_shli(s, dest, p, 32 - cpu.cc_width * 8);
            p = dest;
          }
        default:
          if (p != dest) p = &cpu.cc_dest;
          rtl_setrelop(s, cc2relop_logic[cc], dest, p, rz);
          return;
      }
      break;
    default: panic("unhandle cc_op = %d", cpu.cc_op);
  }
  panic("unhandle cc_op = %d, cc = %d", cpu.cc_op, cc);
negcc_reverse:
    if NEGCC(cc) rtl_xori(s, dest, dest, 1);
    return;
}

static inline def_rtl(lazy_setcc, rtlreg_t *dest, uint32_t cc) {
  if (cpu.cc_dirty == false) { 
    cpu.cc_dynamic = cpu.cc_op | 0x100; 
    // printf("dynamic hit\n");
  }
  rtl_lazy_setcc_internal(s, dest, cc);
}
#endif
