def_EHelper(add) {
  rtl_decode_binary(s, true, true);
  rtlreg_t *res = ddest;
  int need_update_eflags = MUXDEF(CONFIG_x86_CC_NONE, true, s->extraInfo->isa.flag_def != 0);
  if (need_update_eflags) {
    // save dsrc1 firstly cuz maybe $dest = $src
    MUXDEF(CONFIG_x86_CC_LAZY, rtl_mv(s, &cpu.cc_src1, dsrc1), res = s0);
  }
  rtl_add(s, res, ddest, dsrc1);
  if (need_update_eflags) {
#ifdef CONFIG_x86_CC_LAZY
    rtl_set_lazycc(s, res, NULL, NULL, LAZYCC_ADD, s->extraInfo->isa.width);
#else
    rtl_update_ZFSF(s, res, s->extraInfo->isa.width);
    if (s->extraInfo->isa.width != 4) {
      rtl_andi(s, res, res, 0xffffffffu >> ((4 - s->extraInfo->isa.width) * 8));
    }
    rtl_is_add_carry(s, s1, res, ddest);
    rtl_set_CF(s, s1);
    rtl_is_add_overflow(s, s1, res, ddest, dsrc1, s->extraInfo->isa.width);
    rtl_set_OF(s, s1);
#endif
  }
  rtl_wb(s, res);
}

def_EHelper(sub) {
  rtl_decode_binary(s, true, true);
  rtlreg_t *res = ddest;
  int need_update_eflags = MUXDEF(CONFIG_x86_CC_NONE, true, s->extraInfo->isa.flag_def != 0);
  if (need_update_eflags) {
    MUXDEF(CONFIG_x86_CC_LAZY, rtl_set_lazycc(s, ddest, dsrc1, NULL, LAZYCC_SUB, s->extraInfo->isa.width), res = s0);
  }
  rtl_sub(s, res, ddest, dsrc1);
  if (need_update_eflags && !ISDEF(CONFIG_x86_CC_LAZY)) {
    rtl_update_ZFSF(s, res, s->extraInfo->isa.width);
    rtl_is_sub_carry(s, s1, ddest, dsrc1);
    rtl_set_CF(s, s1);
    rtl_is_sub_overflow(s, s1, res, ddest, dsrc1, s->extraInfo->isa.width);
    rtl_set_OF(s, s1);
  }
  rtl_wb(s, res);
}

def_EHelper(cmp) {
  rtl_decode_binary(s, true, true);
  int need_update_eflags = MUXDEF(CONFIG_x86_CC_NONE, true, s->extraInfo->isa.flag_def != 0);
  if (need_update_eflags) {
#ifdef CONFIG_x86_CC_LAZY
    rtl_set_lazycc(s, ddest, dsrc1, NULL, LAZYCC_SUB, s->extraInfo->isa.width);
#else
    rtl_sub(s, s0, ddest, dsrc1);
    rtl_update_ZFSF(s, s0, s->extraInfo->isa.width);
    rtl_is_sub_carry(s, s1, ddest, dsrc1);
    rtl_set_CF(s, s1);
    rtl_is_sub_overflow(s, s1, s0, ddest, dsrc1, s->extraInfo->isa.width);
    rtl_set_OF(s, s1);
#endif
  }
}

def_EHelper(inc) {
  rtl_decode_unary(s, true);
  rtl_addi(s, ddest, ddest, 1);
  int need_update_eflags = MUXDEF(CONFIG_x86_CC_NONE, true, s->extraInfo->isa.flag_def != 0);
  if (need_update_eflags) {
#ifdef CONFIG_x86_CC_LAZY
    // compute CF to cpu.cc_src1
    // rtl_setcc(s, &cpu.cc_src1, CC_B);
    rtl_set_lazycc(s, ddest, NULL, NULL, LAZYCC_INC, s->extraInfo->isa.width);
#else
    rtl_update_ZFSF(s, ddest, s->extraInfo->isa.width);
    rtl_setrelopi(s, RELOP_EQ, s1, ddest, 0x1u << (s->extraInfo->isa.width * 8 - 1));
    rtl_set_OF(s, s1);
#endif
  }
  rtl_wb(s, ddest);
}

def_EHelper(dec) {
  rtl_decode_unary(s, true);
  rtl_subi(s, ddest, ddest, 1);
  int need_update_eflags = MUXDEF(CONFIG_x86_CC_NONE, true, s->extraInfo->isa.flag_def != 0);
  if (need_update_eflags) {
#ifdef CONFIG_x86_CC_LAZY
    // compute CF to cpu.cc_src1
    rtl_lazy_setcc(s, &cpu.cc_src1, CC_B);
    rtl_set_lazycc(s, ddest, NULL, NULL, LAZYCC_DEC, s->extraInfo->isa.width);
#else
    rtl_update_ZFSF(s, ddest, s->extraInfo->isa.width);
    rtl_setrelopi(s, RELOP_EQ, s1, ddest, (0x1u << (s->extraInfo->isa.width * 8 - 1)) - 1);
    rtl_set_OF(s, s1);
#endif
  }
  rtl_wb(s, ddest);
}

def_EHelper(adc) {
  rtl_decode_binary(s, true, true);
  rtlreg_t *res = ddest;
  int need_update_eflags = MUXDEF(CONFIG_x86_CC_NONE, true, s->extraInfo->isa.flag_def != 0);
  rtl_setcc(s, s0, CC_B); // reading CC_B is to read CF
  rtl_add(s, s0, dsrc1, s0);
  if (need_update_eflags) {
    MUXDEF(CONFIG_x86_CC_LAZY, rtl_mv(s, &cpu.cc_src2, dsrc1), res = s1);
  }
    rtl_add(s, res, ddest, s0);
  if (need_update_eflags) {
#ifdef CONFIG_x86_CC_LAZY
    rtl_set_lazycc(s, res, s0, NULL, LAZYCC_ADC, s->extraInfo->isa.width);
#else
    rtl_update_ZFSF(s, res, s->extraInfo->isa.width);
    rtl_is_add_overflow(s, s2, res, ddest, dsrc1, s->extraInfo->isa.width);
    rtl_set_OF(s, s2);
    if (s->extraInfo->isa.width != 4) {
      rtl_andi(s, res, res, 0xffffffffu >> ((4 - s->extraInfo->isa.width) * 8));
    }
    rtl_is_add_carry(s, s2, res, s0);
    rtl_is_add_carry(s, s0, s0, dsrc1);
    rtl_or(s, s0, s0, s2);
    rtl_set_CF(s, s0);
#endif
  }
  rtl_wb(s, res);
}

def_EHelper(sbb) {
  rtl_decode_binary(s, true, true);
  rtlreg_t *res = ddest;
  int need_update_eflags = MUXDEF(CONFIG_x86_CC_NONE, true, s->extraInfo->isa.flag_def != 0);
  rtl_setcc(s, s0, CC_B); // reading CC_B is to read CF
  rtl_add(s, s0, dsrc1, s0);
  if (need_update_eflags) {
#ifdef CONFIG_x86_CC_LAZY
    rtl_mv(s, &cpu.cc_src2, dsrc1);
    rtl_mv(s, &cpu.cc_src1, ddest);
#else
    res = s1;
#endif
  }
  rtl_sub(s, res, ddest, s0);
  if (need_update_eflags) {
#ifdef CONFIG_x86_CC_LAZY
    rtl_set_lazycc(s, res, NULL, NULL, LAZYCC_SBB, s->extraInfo->isa.width);
#else
    rtl_update_ZFSF(s, res, s->extraInfo->isa.width);
    rtl_is_sub_overflow(s, s2, res, ddest, dsrc1, s->extraInfo->isa.width);
    rtl_set_OF(s, s2);
    rtl_is_add_carry(s, s2, s0, dsrc1);
    rtl_is_sub_carry(s, s0, ddest, s0);
    rtl_or(s, s0, s0, s2);
    rtl_set_CF(s, s0);
#endif
  }
  rtl_wb(s, res);
}

def_EHelper(neg) {
  rtl_decode_unary(s, true);
  int need_update_eflags = MUXDEF(CONFIG_x86_CC_NONE, true, s->extraInfo->isa.flag_def != 0);
  if (need_update_eflags) {
#ifdef CONFIG_x86_CC_LAZY
    rtl_sub(s, ddest, rz, ddest);
    rtl_set_lazycc(s, ddest, NULL, NULL, LAZYCC_NEG, s->extraInfo->isa.width);
#else
    rtl_setrelopi(s, RELOP_NE, s1, ddest, 0);
    rtl_set_CF(s, s1);
    rtl_setrelopi(s, RELOP_EQ, s1, ddest, 0x1u << (s->extraInfo->isa.width * 8 - 1));
    rtl_set_OF(s, s1);
    rtl_sub(s, ddest, rz, ddest);
    rtl_update_ZFSF(s, ddest, s->extraInfo->isa.width);
#endif
  } else {
    rtl_sub(s, ddest, rz, ddest);
  }
  rtl_wb(s, ddest);
}

def_EHelper(mul) {
  rtl_decode_unary(s, true);
  int need_update_eflags = MUXDEF(CONFIG_x86_CC_NONE, true, s->extraInfo->isa.flag_def != 0);
  rtlreg_t *hi = s0, *lo = s1;
  switch (s->extraInfo->isa.width) {
    case 1:
      rtl_lr(s, s0, R_EAX, 1);
      rtl_mulu_lo(s, lo, ddest, s0);
      if (need_update_eflags) {
        rtl_andi(s, hi, s1, 0xff00);
      }
      rtl_sr(s, R_AX, lo, 2);
      break;
    case 2:
      rtl_lr(s, s0, R_EAX, 2);
      rtl_mulu_lo(s, lo, ddest, s0);
      rtl_srli(s, hi, lo, 16);
      rtl_sr(s, R_AX, lo, 2);
      rtl_sr(s, R_DX, hi, 2);
      break;
    case 4:
      ; rtlreg_t *pdest = ddest;
      if (ddest == &cpu.edx) {
        rtl_mv(s, s0, ddest);
        pdest = s0;
      }
      hi = &cpu.edx, lo = &cpu.eax;
      rtl_mulu_hi(s, hi, pdest, &cpu.eax);
      rtl_mulu_lo(s, lo, pdest, &cpu.eax);
      break;
    default: assert(0);
  }

  if (need_update_eflags) {
#ifdef CONFIG_x86_CC_LAZY
    rtl_set_lazycc(s, hi, NULL, NULL, LAZYCC_MUL, s->extraInfo->isa.width);
#else
    rtl_setrelopi(s, RELOP_NE, s0, hi, 0);
    rtl_set_OF(s, s0);
    rtl_set_CF(s, s0);
#endif
  }
}

// imul with one operand
def_EHelper(imul1) {
  rtl_decode_unary(s, true);
  int need_update_eflags = MUXDEF(CONFIG_x86_CC_NONE, true, s->extraInfo->isa.flag_def != 0);
  switch (s->extraInfo->isa.width) {
    case 1:
      rtl_lr(s, s0, R_EAX, 1);
      rtl_sext(s, s0, s0, 1);
      rtl_sext(s, ddest, ddest, 1);
      rtl_mulu_lo(s, s1, ddest, s0);
      if (need_update_eflags) {
#if !defined(CONFIG_PA) && !defined(CONFIG_x86_CC_LAZY)
        rtl_sext(s, s0, s1, 1);
        rtl_setrelop(s, RELOP_NE, s0, s0, s1);
        rtl_set_CF(s, s0);
        rtl_set_OF(s, s0);
#endif
      }
      rtl_sr(s, R_AX, s1, 2);
      break;
    case 2:
      rtl_lr(s, s0, R_EAX, 2);
      rtl_sext(s, s0, s0, 2);
      rtl_sext(s, ddest, ddest, 2);
      rtl_mulu_lo(s, s1, ddest, s0);
      if (need_update_eflags) {
#if !defined(CONFIG_PA) && !defined(CONFIG_x86_CC_LAZY)
        rtl_sext(s, s0, s1, 2);
        rtl_setrelop(s, RELOP_NE, s0, s0, s1);
        rtl_set_CF(s, s0);
        rtl_set_OF(s, s0);
#endif
      }
      rtl_sr(s, R_AX, s1, 2);
      rtl_srli(s, s1, s1, 16);
      rtl_sr(s, R_DX, s1, 2);
      break;
    case 4:
      ; rtlreg_t *pdest = ddest;
      if (ddest == &cpu.edx) {
        rtl_mv(s, s0, ddest);
        pdest = s0;
      }
      rtl_muls_hi(s, &cpu.edx, pdest, &cpu.eax);
      rtl_mulu_lo(s, &cpu.eax, pdest, &cpu.eax);
      if (need_update_eflags) {
#if !defined(CONFIG_PA) && !defined(CONFIG_x86_CC_LAZY)
        rtl_msb(s, s0, &cpu.eax, 4);
        rtl_add(s, s0, &cpu.edx, s0);
        rtl_setrelopi(s, RELOP_NE, s0, s0, 0);
        rtl_set_CF(s, s0);
        rtl_set_OF(s, s0);
#endif
      }
      break;
    default: assert(0);
  }
}

// imul with two operands
def_EHelper(imul2) {
  rtl_decode_binary(s, true, true);
  rtl_sext(s, dsrc1, dsrc1, s->extraInfo->isa.width);
  rtl_sext(s, ddest, ddest, s->extraInfo->isa.width);

#if !defined(CONFIG_PA) && !defined(CONFIG_x86_CC_LAZY)
  int need_update_eflags = MUXDEF(CONFIG_x86_CC_NONE, true, s->extraInfo->isa.flag_def != 0);
  if (need_update_eflags) {
    if (s->extraInfo->isa.width == 4) { rtl_muls_hi(s, s1, ddest, dsrc1); }
  }
#endif

  rtl_mulu_lo(s, ddest, ddest, dsrc1);

#if !defined(CONFIG_PA) && !defined(CONFIG_x86_CC_LAZY)
  if (need_update_eflags) {
    if (s->extraInfo->isa.width == 2) {
      rtl_sext(s, s0, ddest, s->extraInfo->isa.width);
      rtl_setrelop(s, RELOP_NE, s0, s0, ddest);
    } else if (s->extraInfo->isa.width == 4) {
      rtl_msb(s, s0, ddest, s->extraInfo->isa.width);
      rtl_add(s, s0, s1, s0);
      rtl_setrelopi(s, RELOP_NE, s0, s0, 0);
    } else {
      assert(0);
    }
    rtl_set_CF(s, s0);
    rtl_set_OF(s, s0);
  }
#endif

  rtl_wb_r(s, ddest);
}

// imul with three operands
def_EHelper(imul3) {
  rtl_decode_binary(s, false, true); // the 3rd operand is always immediate
  rt_decode(s, id_src2, true, s->extraInfo->isa.width);
  rtl_sext(s, dsrc1, dsrc1, s->extraInfo->isa.width);

  int need_update_eflags = MUXDEF(CONFIG_x86_CC_NONE, true, s->extraInfo->isa.flag_def != 0);
  if (need_update_eflags) {
#if !defined(CONFIG_PA) && !defined(CONFIG_x86_CC_LAZY)
    if (s->extraInfo->isa.width == 4) { rtl_muls_hi(s, s1, dsrc1, dsrc2); }
#endif
  }

  rtl_mulu_lo(s, ddest, dsrc1, dsrc2);

  if (need_update_eflags) {
#if !defined(CONFIG_PA) && !defined(CONFIG_x86_CC_LAZY)
    if (s->extraInfo->isa.width == 2) {
      rtl_sext(s, s0, ddest, s->extraInfo->isa.width);
      rtl_setrelop(s, RELOP_NE, s0, s0, ddest);
    } else if (s->extraInfo->isa.width == 4) {
      rtl_msb(s, s0, ddest, s->extraInfo->isa.width);
      rtl_add(s, s0, s1, s0);
      rtl_setrelopi(s, RELOP_NE, s0, s0, 0);
    } else {
      assert(0);
    }
    rtl_set_CF(s, s0);
    rtl_set_OF(s, s0);
#endif
  }

  rtl_wb_r(s, ddest);
}

def_EHelper(div) {
  rtl_decode_unary(s, true);
  switch (s->extraInfo->isa.width) {
    case 1:
      rtl_lr(s, s0, R_AX, 2);
      rtl_divu_q(s, s1, s0, ddest);
      rtl_divu_r(s, s0, s0, ddest);
      rtl_sr(s, R_AL, s1, 1);
      rtl_sr(s, R_AH, s0, 1);
      break;
    case 2:
      rtl_lr(s, s0, R_AX, 2);
      rtl_lr(s, s1, R_DX, 2);
      rtl_slli(s, s1, s1, 16);
      rtl_or(s, s0, s0, s1);
      rtl_divu_q(s, s1, s0, ddest);
      rtl_divu_r(s, s0, s0, ddest);
      rtl_sr(s, R_AX, s1, 2);
      rtl_sr(s, R_DX, s0, 2);
      break;
    case 4:
      ; rtlreg_t *pdest = ddest;
      if (ddest == &cpu.eax) pdest = s0;
      rtl_mv(s, s0, &cpu.eax);
      rtl_div64u_q(s, &cpu.eax, &cpu.edx, s0, pdest);
      rtl_div64u_r(s, &cpu.edx, &cpu.edx, s0, pdest);
      break;
    default: assert(0);
  }
}

def_EHelper(idiv) {
  rtl_decode_unary(s, true);
  switch (s->extraInfo->isa.width) {
    case 1:
      rtl_lr(s, s0, R_AX, 2);
      rtl_divs_q(s, s1, s0, ddest);
      rtl_divs_r(s, s0, s0, ddest);
      rtl_sr(s, R_AL, s1, 1);
      rtl_sr(s, R_AH, s0, 1);
      break;
    case 2:
      rtl_lr(s, s0, R_AX, 2);
      rtl_lr(s, s1, R_DX, 2);
      rtl_slli(s, s1, s1, 16);
      rtl_or(s, s0, s0, s1);
      rtl_divs_q(s, s1, s0, ddest);
      rtl_divs_r(s, s0, s0, ddest);
      rtl_sr(s, R_AX, s1, 2);
      rtl_sr(s, R_DX, s0, 2);
      break;
    case 4:
      ; rtlreg_t *pdest = ddest;
      if (ddest == &cpu.eax) pdest = s0;
      rtl_mv(s, s0, &cpu.eax);
      rtl_div64s_q(s, &cpu.eax, &cpu.edx, s0, pdest);
      rtl_div64s_r(s, &cpu.edx, &cpu.edx, s0, pdest);
      break;
    default: assert(0);
  }
}

def_EHelper(xadd) {
  rtl_decode_binary(s, true, true);
  int need_update_eflags = MUXDEF(CONFIG_x86_CC_NONE, true, s->extraInfo->isa.flag_def != 0);
  rtl_add(s, s0, ddest, dsrc1);
  if (need_update_eflags) {
#ifdef CONFIG_x86_CC_LAZY
    rtl_set_lazycc(s, s0, dsrc1, NULL, LAZYCC_ADD, s->extraInfo->isa.width);
#else
    rtl_update_ZFSF(s, s0, s->extraInfo->isa.width);
    if (s->extraInfo->isa.width != 4) {
      rtl_andi(s, s0, s0, 0xffffffffu >> ((4 - s->extraInfo->isa.width) * 8));
    }
    rtl_is_add_carry(s, s1, s0, ddest);
    rtl_set_CF(s, s1);
    rtl_is_add_overflow(s, s1, s0, ddest, dsrc1, s->extraInfo->isa.width);
    rtl_set_OF(s, s1);
#endif
  }
  rtl_sr(s, id_src1->reg, ddest, s->extraInfo->isa.width);
  rtl_wb(s, s0);
}
