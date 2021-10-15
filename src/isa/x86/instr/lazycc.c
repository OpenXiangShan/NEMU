#include "lazycc.h"

#ifdef CONFIG_x86_CC_LAZY
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
  [CC_P]  = UNARY,             [CC_NP]  = UNARY,
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

static def_rtl(setrelop_or_jrelop, CCop *op) {
  switch (op->type) {
    case CCTYPE_SETCC: rtl_setrelop(s, op->relop, op->dest, op->src1, op->src2); break;
    // call rtl_relop() in exec_jcc() to let lazycc works with performance optimization mode
    case CCTYPE_JCC: break;
    default: assert(0);
  }
}

static def_rtl(lazycc_internal, CCop *op, uint32_t cc) {
  rtlreg_t *p = NULL;
  rtlreg_t *tmp = (op->type == CCTYPE_SETCC ? op->dest : s2);
  int exception = (cpu.cc_op == LAZYCC_SUB || cpu.cc_op == LAZYCC_FCMP || cpu.cc_op == LAZYCC_FCMP_SAME)
    && (cc == CC_E || cc == CC_NE);
  if ((cc2relop[cc] & UNARY) && !exception) {
    p = &cpu.cc_dest;
    if (cpu.cc_op == LAZYCC_SUB) {
      // sub && (CC_S || CC_NS || CC_P || CC_NP)
      // should compute the real result of the sub
      rtl_sub(s, tmp, &cpu.cc_dest, &cpu.cc_src1);
      p = tmp;
    }
    if (cc == CC_P || cc == CC_NP) {
      if (cpu.cc_op == LAZYCC_FCMP || cpu.cc_op == LAZYCC_FCMP_SAME) {
        op->relop = (cc == CC_P ? RELOP_FALSE : RELOP_TRUE);
        rtl_setrelop_or_jrelop(s, op);
        return;
      }
      // start:                     p  = ????76543210
      rtl_srli(s, t0, p, 4);    //  t0 =     ????7654
      rtl_xor(s, t0, t0, p);    //  t0 =    ????(7^3)(6^2)(5^1)(4^0)
      rtl_srli(s, tmp, t0, 2);  // tmp =      ??  ?    ?  (7^3)(6^2)
      rtl_xor(s, t0, t0, tmp);  //  t0 =          ?    ?  (7^3^5^1)(6^2^4^0)
      rtl_srli(s, tmp, t0, 1);  // tmp =                      ?    (7^3^5^1)
      rtl_xor(s, t0, t0, tmp);  //  t0 =                      ?    (7^6^5^4^3^2^1^0)
      rtl_andi(s, tmp, t0, 1);  // tmp = !PF
      op->relop = (cc == CC_P ? RELOP_EQ : RELOP_NE);
      op->src1 = tmp;
      op->src2 = rz;
      rtl_setrelop_or_jrelop(s, op);
      return;
    }
    int exception = (cpu.cc_op == LAZYCC_LOGIC) && (cc == CC_E || cc == CC_NE);
    if (cpu.cc_width != 4 && !exception) {
      rtl_slli(s, tmp, p, 32 - cpu.cc_width * 8);
      p = tmp;
    }
    op->relop = cc2relop[cc] ^ UNARY;
    op->src1 = p;
    op->src2 = rz;
    rtl_setrelop_or_jrelop(s, op);
    return;
  }

  switch (cpu.cc_op) {
    case LAZYCC_ADD:
      switch (cc) {
        case CC_O: case CC_NO:
#if 0
          rtl_sub(s, dest, &cpu.cc_dest, &cpu.cc_src1);
          rtl_is_add_overflow(s, dest, &cpu.cc_dest, &cpu.cc_src1, dest, cpu.cc_width);
          goto negcc_reverse;
          return;
#endif
        case CC_LE: case CC_NLE:
#if 0
          rtl_sub(s, dest, &cpu.cc_dest, &cpu.cc_src1);
          rtl_is_add_overflow(s, s0, &cpu.cc_dest, &cpu.cc_src1, dest, cpu.cc_width);
          rtl_msb(s, s1, &cpu.cc_dest, cpu.cc_width);
          rtl_xor(s, dest, s0, s1);
          MASKDEST(s0);
          rtl_setrelopi(s, RELOP_EQ, s0, p, 0);
          rtl_or(s, dest, dest, s0);
          goto negcc_reverse;
          return;
#endif
        case CC_BE: case CC_NBE:
#if 0
          MASKDEST(s0);
          rtl_is_add_carry(s, s1, p, &cpu.cc_src1);
          rtl_setrelopi(s, RELOP_EQ, s0, p, 0);
          rtl_or(s, dest, s0, s1);
          goto negcc_reverse;
          return;
#endif
          panic("TODO: pc = 0x%x", s->pc);
        default:
          if (cc2relop[cc] != 0) {
            op->relop = cc2relop[cc];
            op->src1 = &cpu.cc_dest;
            op->src2 = &cpu.cc_src1;
            rtl_setrelop_or_jrelop(s, op);
            return;
          }
      }
      break;
    case LAZYCC_SUB:
      switch (cc) {
        case CC_O: case CC_NO:
          panic("TODO: pc = 0x%x", s->pc);
#if 0
          rtl_sub(s, dest, &cpu.cc_dest, &cpu.cc_src1);
          rtl_is_sub_overflow(s, dest, dest, &cpu.cc_dest, &cpu.cc_src1, cpu.cc_width);
          goto negcc_reverse;
#endif
          return;
        default:
          if (cc2relop[cc] != 0) {
            rtlreg_t *pdest = &cpu.cc_dest;
            rtlreg_t *psrc1 = &cpu.cc_src1;
            if (cpu.cc_width != 4) {
              if (cc == CC_L || cc == CC_NL || cc == CC_LE || cc == CC_NLE) {
                rtl_slli(s, tmp, pdest, 32 - cpu.cc_width * 8);
                rtl_slli(s, t0,  psrc1, 32 - cpu.cc_width * 8);
                pdest = tmp;
                psrc1 = t0;
              }
            }
            //rtl_setrelop(s, cc2relop[cc] & 0xf, dest, &cpu.cc_dest, &cpu.cc_src1);
            op->relop = cc2relop[cc] & 0xf;
            op->src1 = pdest;
            op->src2 = psrc1;
            rtl_setrelop_or_jrelop(s, op);
            return;
          }
      }
      break;
    case LAZYCC_NEG:
      switch (cc) {
        case CC_B: //case CC_NB:
          op->relop = RELOP_NE;
          op->src1 = &cpu.cc_dest;
          op->src2 = rz;
          rtl_setrelop_or_jrelop(s, op);
          return;
#if 0
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
#endif
      }
      break;
    case LAZYCC_INC:
      switch (cc) {
        case CC_B: case CC_NB: // CF is already stored in cpu.cc_src1
          op->relop = (cc == CC_NB ? RELOP_EQ : RELOP_NE);
          op->src1 = &cpu.cc_src1;
          op->src2 = rz;
          rtl_setrelop_or_jrelop(s, op);
          return;
#if 0
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
#endif
      }
      break;
    case LAZYCC_DEC:
      switch (cc) {
        case CC_B: case CC_NB:  // CF is already stored in cpu.cc_src1
          op->relop = (cc == CC_NB ? RELOP_EQ : RELOP_NE);
          op->src1 = &cpu.cc_src1;
          op->src2 = rz;
          rtl_setrelop_or_jrelop(s, op);
          return;
#if 0
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
#endif
      }
      break;
    case LAZYCC_ADC:
      switch (cc) {
        case CC_B: case CC_NB:
          rtl_is_add_carry(s, s0, &cpu.cc_src1, &cpu.cc_src2);
          rtl_is_add_carry(s, s1, &cpu.cc_dest, &cpu.cc_src1);
          rtl_or(s, tmp, s0, s1);
          if (op->type == CCTYPE_JCC) {
            op->relop = (cc == CC_NB ? RELOP_EQ : RELOP_NE);
            op->src1 = tmp;
            op->src2 = rz;
          } else {
            if (cc == CC_NB) rtl_xori(s, tmp, tmp, 1);
          }
          return;
#if 0
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
#endif
      }
      break;
    case LAZYCC_SBB:
      switch (cc) {
        case CC_B: case CC_NB:
          rtl_sub(s, s0, &cpu.cc_src1, &cpu.cc_dest);
          rtl_is_add_carry(s, s0, s0, &cpu.cc_src2);
          rtl_is_sub_carry(s, s1, &cpu.cc_src1, &cpu.cc_dest);
          rtl_or(s, tmp, s0, s1);
          if (op->type == CCTYPE_JCC) {
            op->relop = (cc == CC_NB ? RELOP_EQ : RELOP_NE);
            op->src1 = tmp;
            op->src2 = rz;
          } else {
            if (cc == CC_NB) rtl_xori(s, tmp, tmp, 1);
          }
          return;
#if 0
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
#endif
      }
      break;
    case LAZYCC_LOGIC:
      switch (cc) {
        case CC_LE: case CC_NLE: case CC_L: case CC_NL:
          p = &cpu.cc_dest;
          if (cpu.cc_width != 4) {
            rtl_slli(s, tmp, p, 32 - cpu.cc_width * 8);
            p = tmp;
          }
        default:
          if (p != tmp) p = &cpu.cc_dest;
          op->relop = cc2relop_logic[cc];
          assert(op->relop != 0);
          op->src1 = p;
          op->src2 = rz;
          rtl_setrelop_or_jrelop(s, op);
          return;
      }
      break;
    case LAZYCC_SHL:
      switch (cc) {
        case CC_B: case CC_NB:
          rtl_msb(s, tmp, &cpu.cc_src1, cpu.cc_width);
          op->relop = (cc == CC_NB ? RELOP_EQ : RELOP_NE);
          op->src1 = tmp;
          op->src2 = rz;
          rtl_setrelop_or_jrelop(s, op);
          return;
      }
      break;
    case LAZYCC_SHR:
      switch (cc) {
        case CC_B: case CC_NB:
shift_right_cf:
          rtl_andi(s, tmp, &cpu.cc_src1, 0x1);
          op->relop = (cc == CC_NB ? RELOP_EQ : RELOP_NE);
          op->src1 = tmp;
          op->src2 = rz;
          rtl_setrelop_or_jrelop(s, op);
          return;
      }
      break;
    case LAZYCC_SAR:
      switch (cc) {
        case CC_B: case CC_NB:
          goto shift_right_cf;
      }
      break;
    case LAZYCC_BT:
      switch (cc) {
        case CC_B: case CC_NB:
          op->relop = (cc == CC_NB ? RELOP_EQ : RELOP_NE);
          // cc_dest == rz --> CF = 0 -> CC_NB
          op->src1 = &cpu.cc_dest;
          op->src2 = rz;
          rtl_setrelop_or_jrelop(s, op);
          return;
      }
      break;
    case LAZYCC_FCMP:
      switch (cc) {
        case CC_E: case CC_NE: rtl_feqd(s, tmp, &cpu.cc_fp_dest, &cpu.cc_fp_src1);
          if (op->type == CCTYPE_SETCC) {
            if (cc == CC_NE) rtl_xori(s, tmp, tmp, 1);
          } else {
            op->relop = (cc == CC_NE ? RELOP_EQ : RELOP_NE);
            op->src1 = tmp;
            op->src2 = rz;
          }
          return;
        case CC_B:   rtl_fltd(s, tmp, &cpu.cc_fp_dest, &cpu.cc_fp_src1); goto fcmp_check_jcc;
        case CC_BE:  rtl_fled(s, tmp, &cpu.cc_fp_dest, &cpu.cc_fp_src1); goto fcmp_check_jcc;
        case CC_NB:  rtl_fled(s, tmp, &cpu.cc_fp_src1, &cpu.cc_fp_dest); goto fcmp_check_jcc;
        case CC_NBE: rtl_fltd(s, tmp, &cpu.cc_fp_src1, &cpu.cc_fp_dest); goto fcmp_check_jcc;
        fcmp_check_jcc:
          if (op->type == CCTYPE_JCC) {
            op->relop = RELOP_NE;
            op->src1 = tmp;
            op->src2 = rz;
          }
          return;
      }
      break;
    case LAZYCC_FCMP_SAME:
      switch (cc) {
        case CC_NE: op->relop = RELOP_FALSE; return;
      }
      break;
    case LAZYCC_MUL:
      switch (cc) {
        case CC_O:
          op->relop = RELOP_NE;
          op->src1 = &cpu.cc_dest;
          op->src2 = rz;
          rtl_setrelop_or_jrelop(s, op);
          return;
      }
      break;
    default: panic("unhandle cc_op = %d at pc = " FMT_WORD, cpu.cc_op, s->pc);
  }
  panic("unhandle cc_op = %d, cc = %d at pc = " FMT_WORD, cpu.cc_op, cc, s->pc);
#if 0
negcc_reverse:
    if NEGCC(cc) rtl_xori(s, dest, dest, 1);
#endif
    return;
}

def_rtl(lazy_setcc, rtlreg_t *dest, uint32_t cc) {
  if (cpu.cc_dirty == false) {
    cpu.cc_dynamic = cpu.cc_op | 0x100;
    // printf("dynamic hit\n");
  }
  CCop op;
  op.dest = dest;
  op.type = CCTYPE_SETCC;
  rtl_lazycc_internal(s, &op, cc);
}

def_rtl(lazy_jcc, CCop *op, uint32_t cc) {
  if (cpu.cc_dirty == false) {
    cpu.cc_dynamic = cpu.cc_op | 0x100;
  }
  clean_lazycc();
  op->type = CCTYPE_JCC;
  rtl_lazycc_internal(s, op, cc);
}
#endif
