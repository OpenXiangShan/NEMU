#define def_logic_template(name) \
  def_EHelper(name) { \
    rtl_decode_binary(s, true, true); \
    concat(rtl_, name)(s, ddest, ddest, dsrc1); \
    int need_update_eflags = MUXDEF(CONFIG_x86_CC_NONE, true, s->extraInfo->isa.flag_def != 0); \
    if (need_update_eflags) { \
      IFDEF(CONFIG_x86_CC_LAZY, rtl_set_lazycc(s, ddest, NULL, NULL, LAZYCC_LOGIC, s->extraInfo->isa.width)); \
      if (!ISDEF(CONFIG_x86_CC_LAZY)) { \
        rtl_update_ZFSF(s, ddest, s->extraInfo->isa.width); \
        rtl_mv(s, &cpu.CF, rz); \
        rtl_mv(s, &cpu.OF, rz); \
      } \
    } \
    rtl_wb(s, ddest); \
  }

def_logic_template(and)
def_logic_template(or)
def_logic_template(xor)

def_EHelper(test) {
  rtl_decode_binary(s, true, true);
  rtl_and(s, s0, ddest, dsrc1);
  int need_update_eflags = MUXDEF(CONFIG_x86_CC_NONE, true, s->extraInfo->isa.flag_def != 0);
  if (need_update_eflags) {
#ifdef CONFIG_x86_CC_LAZY
    rtl_set_lazycc(s, s0, NULL, NULL, LAZYCC_LOGIC, s->extraInfo->isa.width);
#else
    rtl_update_ZFSF(s, s0, s->extraInfo->isa.width);
    rtl_mv(s, &cpu.CF, rz);
    rtl_mv(s, &cpu.OF, rz);
#endif
  }
}

def_EHelper(not) {
  rtl_decode_unary(s, true);
  rtl_not(s, ddest, ddest);
  rtl_wb(s, ddest);
}

def_EHelper(setcc) {
  rtl_decode_unary(s, false);
  uint32_t cc = s->extraInfo->isa.opcode & 0xf;
  rtl_setcc(s, ddest, cc);
  rtl_wb(s, ddest);
}

def_EHelper(shl) {
  rtl_decode_binary(s, true, true);
  rtlreg_t *tmp = MUXDEF(CONFIG_x86_CC_LAZY, &cpu.cc_src1, s1);
  int need_update_eflags = MUXDEF(CONFIG_x86_CC_NONE, true, s->extraInfo->isa.flag_def != 0);
  if (need_update_eflags) {
    rtl_subi(s, s0, dsrc1, 1);
    rtl_sll(s, tmp, ddest, s0); // shift (cnt - 1)
  }
  rtl_sll(s, ddest, ddest, dsrc1);
  if (need_update_eflags) {
#ifdef CONFIG_x86_CC_LAZY
    rtl_set_lazycc(s, ddest, NULL, NULL, LAZYCC_SHL, s->extraInfo->isa.width);
#else
    rtl_msb(s, s0, tmp, s->extraInfo->isa.width);
    rtl_set_CF(s, s0);
    rtl_update_ZFSF(s, ddest, s->extraInfo->isa.width);
    if (MUXDEF(CONFIG_DIFFTEST_REF_KVM, count == 1, 1)) {
      rtl_xor(s, s0, tmp, ddest);
      rtl_msb(s, s0, s0, s->extraInfo->isa.width);
      rtl_set_OF(s, s0);
    }
#endif
  }
  rtl_wb(s, ddest);
}

def_EHelper(shr) {
  rtl_decode_binary(s, true, true);
  rtlreg_t *tmp = MUXDEF(CONFIG_x86_CC_LAZY, &cpu.cc_src1, s1);
  int need_update_eflags = MUXDEF(CONFIG_x86_CC_NONE, true, s->extraInfo->isa.flag_def != 0);
  if (need_update_eflags) {
    rtl_subi(s, s0, dsrc1, 1);
    rtl_srl(s, tmp, ddest, s0); // shift (cnt - 1)
  }
  rtl_srl(s, ddest, ddest, dsrc1);
  if (need_update_eflags) {
#ifdef CONFIG_x86_CC_LAZY
    rtl_set_lazycc(s, ddest, NULL, NULL, LAZYCC_SHR, s->extraInfo->isa.width);
#else
    rtl_andi(s, s0, tmp, 0x1);
    rtl_set_CF(s, s0);
    rtl_update_ZFSF(s, ddest, s->extraInfo->isa.width);
    if (MUXDEF(CONFIG_DIFFTEST_REF_KVM, count == 1, 1)) {
      rtl_xor(s, s0, tmp, ddest);
      rtl_msb(s, s0, s0, s->extraInfo->isa.width);
      rtl_set_OF(s, s0);
    }
#endif
  }
  rtl_wb(s, ddest);
}

def_EHelper(sar) {
  rtl_decode_binary(s, true, true);
  rtlreg_t *tmp = MUXDEF(CONFIG_x86_CC_LAZY, &cpu.cc_src1, s1);
  int need_update_eflags = MUXDEF(CONFIG_x86_CC_NONE, true, s->extraInfo->isa.flag_def != 0);

  // if ddest == dsrc1, rtl_sra() still only use the
  // lower 5 bits of dsrc1, which do not change after
  // rtl_sext(), and it is still sematically correct
  rtl_sext(s, ddest, ddest, s->extraInfo->isa.width);
  if (need_update_eflags) {
    rtl_subi(s, s0, dsrc1, 1);
    rtl_sra(s, tmp, ddest, s0); // shift (cnt - 1)
  }
  rtl_sra(s, ddest, ddest, dsrc1);
  if (need_update_eflags) {
#ifdef CONFIG_x86_CC_LAZY
    rtl_set_lazycc(s, ddest, NULL, NULL, LAZYCC_SAR, s->extraInfo->isa.width);
#else
    rtl_andi(s, s0, tmp, 0x1);
    rtl_set_CF(s, s0);
    rtl_update_ZFSF(s, ddest, s->extraInfo->isa.width);
    if (MUXDEF(CONFIG_DIFFTEST_REF_KVM, count == 1, 1)) {
      rtl_xor(s, s0, tmp, ddest);
      rtl_msb(s, s0, s0, s->extraInfo->isa.width);
      rtl_set_OF(s, s0);
    }
#endif
  }
  rtl_wb(s, ddest);
}

def_EHelper(rol) {
  rtl_decode_binary(s, true, true);
  rtl_sll(s, s0, ddest, dsrc1);
  rtl_li(s, s1, s->extraInfo->isa.width * 8);
  rtl_sub(s, s1, s1, dsrc1);
  rtl_srl(s, s1, ddest, s1);
  rtl_or(s, ddest, s0, s1);
  rtl_wb(s, ddest);
  // unnecessary to update eflags in NEMU
  //difftest_skip_eflags(EFLAGS_MASK_ALL);
}

def_EHelper(ror) {
  rtl_decode_binary(s, true, true);
  rtl_srl(s, s0, ddest, dsrc1);
  rtl_li(s, s1, s->extraInfo->isa.width * 8);
  rtl_sub(s, s1, s1, dsrc1);
  rtl_sll(s, s1, ddest, s1);
  rtl_or(s, ddest, s0, s1);
  rtl_wb(s, ddest);
  // unnecessary to update eflags in NEMU
  //difftest_skip_eflags(EFLAGS_MASK_ALL);
}

def_EHelper(shld) {
  assert(s->extraInfo->isa.width == 4);
  rtl_decode_binary(s, true, true);
  rt_decode(s, id_src2, true, s->extraInfo->isa.width);
  rtlreg_t *tmp = MUXDEF(CONFIG_x86_CC_LAZY, &cpu.cc_src1, s1);
  int need_update_eflags = MUXDEF(CONFIG_x86_CC_NONE, true, s->extraInfo->isa.flag_def != 0);
  if (need_update_eflags) {
    rtl_subi(s, s0, dsrc2, 32);
    rtl_sub(s, s0, rz, s0);
    rtl_srl(s, tmp, ddest, s0); // shift (cnt - 1)
  }

  rtl_sll(s, s0, ddest, dsrc2);

  rtl_li(s, s2, 31);
  rtl_sub(s, s2, s2, dsrc2);
  // shift twice to deal with dsrc1 = 0
  // the first shift is still right even if we do not
  // mask out the high part of `dsrc1`, since we have
  //   (31 - (dsrc1 & 0x1f)) = (31 - dsrc1 % 32) = (31 - dsrc1) mod 32
  rtl_srl(s, s2, dsrc1, s2);
  rtl_srli(s, s2, s2, 1);

  rtl_or(s, ddest, s0, s2);
  rtl_wb(s, ddest);

  if (need_update_eflags) {
#ifdef CONFIG_x86_CC_LAZY
    rtl_set_lazycc(s, ddest, NULL, NULL, LAZYCC_SHL, s->extraInfo->isa.width);
#else
    rtl_andi(s, s0, tmp, 0x1);
    rtl_set_CF(s, s0);
    rtl_update_ZFSF(s, ddest, s->extraInfo->isa.width);
#endif
  }
}

def_EHelper(shrd) {
  assert(s->extraInfo->isa.width == 4);
  rtl_decode_binary(s, true, true);
  rt_decode(s, id_src2, true, s->extraInfo->isa.width);
  rtlreg_t *tmp = MUXDEF(CONFIG_x86_CC_LAZY, &cpu.cc_src1, s1);
  int need_update_eflags = MUXDEF(CONFIG_x86_CC_NONE, true, s->extraInfo->isa.flag_def != 0);

#ifdef CONFIG_ENGINE_INTERPRETER
  int count = *dsrc2 & 0x1f;
  if (count == 0) {
    rtl_wb(s, ddest);
    goto end;
    //assert(0);
    //return;
  }
#endif

  if (need_update_eflags) {
    rtl_subi(s, s0, dsrc2, 1);
    rtl_srl(s, tmp, ddest, s0); // shift (cnt - 1)
  }

  rtl_srl(s, s0, ddest, dsrc2);
  rtl_li(s, s2, 31);
  rtl_sub(s, s2, s2, dsrc2);
  // shift twice to deal with dsrc1 = 0
  // the first shift is still right even if we do not
  // mask out the high part of `dsrc1`, since we have
  //   (31 - (dsrc1 & 0x1f)) = (31 - dsrc1 % 32) = (31 - dsrc1) mod 32
  rtl_sll(s, s2, dsrc1, s2);
  rtl_slli(s, s2, s2, 1);

  rtl_or(s, ddest, s0, s2);
  rtl_wb(s, ddest);

  if (need_update_eflags) {
#ifdef CONFIG_x86_CC_LAZY
    rtl_set_lazycc(s, ddest, NULL, NULL, LAZYCC_SHR, s->extraInfo->isa.width);
#else
    rtl_andi(s, s0, tmp, 0x1);
    rtl_set_CF(s, s0);
    rtl_update_ZFSF(s, ddest, s->extraInfo->isa.width);
    //if (MUXDEF(CONFIG_DIFFTEST_REF_KVM, count == 1, 1)) {
    //  rtl_xor(s, s0, tmp, ddest);
    //  rtl_msb(s, s0, s0, s->isa.width);
    //  rtl_set_OF(s, s0);
    //}
#endif
  }
end: ;
}

#if 0
static inline def_EHelper(rcr) {
  rtl_srl(s, s0, ddest, dsrc1);

  rtl_get_CF(s, s1);
  rtl_slli(s, s1, s1, 31);
  rtl_srl(s, s1, s1, dsrc1);
  rtl_slli(s, s1, s1, 1);
  rtl_or(s, s0, s0, s1);

  rtl_li(s, s1, 1);
  rtl_sll(s, s1, s1, dsrc1);
  rtl_srli(s, s1, s1, 1);
  rtl_and(s, s1, ddest, s1);
  rtl_setrelopi(s, RELOP_NE, s1, s1, 0);
  rtl_set_CF(s, s1);

  rtl_li(s, s1, id_dest->s->extraInfo->isa.width * 8);
  rtl_sub(s, s1, s1, dsrc1);
  rtl_sll(s, s1, ddest, s1);
  rtl_slli(s, s1, s1, 1);
  rtl_or(s, ddest, s0, s1);

  operand_write(s, id_dest, ddest);
}

static inline def_EHelper(rcl) {
  rtl_sll(s, s0, ddest, dsrc1);

  rtl_get_CF(s, s1);
  rtl_sll(s, s1, s1, dsrc1);
  rtl_srli(s, s1, s1, 1);
  rtl_or(s, s0, s0, s1);

  rtl_li(s, s1, 0x80000000);
  rtl_srl(s, s1, s1, dsrc1);
  rtl_slli(s, s1, s1, 1);
  rtl_and(s, s1, ddest, s1);
  rtl_setrelopi(s, RELOP_NE, s1, s1, 0);
  rtl_set_CF(s, s1);

  rtl_li(s, s1, id_dest->s->extraInfo->isa.width * 8);
  rtl_sub(s, s1, s1, dsrc1);
  rtl_srl(s, s1, ddest, s1);
  rtl_srli(s, s1, s1, 1);
  rtl_or(s, ddest, s0, s1);

  operand_write(s, id_dest, ddest);
}
#endif
