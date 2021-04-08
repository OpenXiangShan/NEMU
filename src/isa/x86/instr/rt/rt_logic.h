// dest <- and result
static inline void and_internal(Decode *s, int width) {
  rtl_and(s, s0, ddest, dsrc1);
  rtl_update_ZFSF(s, s0, width);
  rtl_mv(s, &cpu.CF, rz);
  rtl_mv(s, &cpu.OF, rz);
}

def_REHelper(and) {
#ifdef LAZY_CC
  rtl_and(s, ddest, ddest, dsrc1);
  rtl_set_lazycc(s, ddest, NULL, NULL, LAZYCC_LOGIC, width);
#else
  and_internal(s, width);
  rtl_mv(s, ddest, s0);
#endif
}

def_REHelper(or) {
  rtl_or(s, ddest, ddest, dsrc1);
#ifdef LAZY_CC
  rtl_set_lazycc(s, ddest, NULL, NULL, LAZYCC_LOGIC, width);
#else
  rtl_update_ZFSF(s, ddest, width);
  rtl_mv(s, &cpu.CF, rz);
  rtl_mv(s, &cpu.OF, rz);
#endif
}


def_REHelper(test) {
#ifdef LAZY_CC
  rtl_and(s, s0, ddest, dsrc1);
  rtl_set_lazycc(s, s0, NULL, NULL, LAZYCC_LOGIC, width);
#else
  and_internal(s, width);
#endif
}

def_REHelper(xor) {
  rtl_xor(s, ddest, ddest, dsrc1);
#ifdef LAZY_CC
  rtl_set_lazycc(s, ddest, NULL, NULL, LAZYCC_LOGIC, width);
#else
  rtl_update_ZFSF(s, ddest, width);
  rtl_mv(s, &cpu.CF, rz);
  rtl_mv(s, &cpu.OF, rz);
#endif
}

def_REHelper(not) {
  rtl_not(s, ddest, ddest);
}

def_REHelper(setcc) {
  uint32_t cc = s->isa.opcode & 0xf;
#ifdef LAZY_CC
  rtl_lazy_setcc(s, ddest, cc);
#else
  rtl_setcc(s, ddest, cc);
#endif
}

def_REHelper(shl) {
#ifndef __PA__
#ifdef CONFIG_ENGINE_INTERPRETER
  int count = *dsrc1 & 0x1f;
  if (count == 0) return;
#endif

  rtl_subi(s, s0, dsrc1, 1);
  rtl_shl(s, s1, ddest, s0); // shift (cnt - 1)
  rtl_msb(s, s0, s1, width);
  rtl_set_CF(s, s0);
  rtl_shl(s, ddest, ddest, dsrc1);

  if (MUXDEF(CONFIG_DIFFTEST_REF_KVM, count == 1, 1)) {
    rtl_xor(s, s0, s1, ddest);
    rtl_msb(s, s0, s0, width);
    rtl_set_OF(s, s0);
  }

  rtl_update_ZFSF(s, ddest, width);
#else
  rtl_shl(s, ddest, ddest, dsrc1);
  rtl_update_ZFSF(s, ddest, width);
#endif
#ifdef LAZY_CC
  //panic("TODO: implement CF and OF with lazy cc");
#endif
}

def_REHelper(shr) {
#ifndef __PA__
#ifdef CONFIG_ENGINE_INTERPRETER
  int count = *dsrc1 & 0x1f;
  if (count == 0) return;
#endif

  rtl_subi(s, s0, dsrc1, 1);
  rtl_shr(s, s1, ddest, s0); // shift (cnt - 1)
  rtl_andi(s, s0, s1, 0x1);
  rtl_set_CF(s, s0);
  rtl_shr(s, ddest, ddest, dsrc1);

  if (MUXDEF(CONFIG_DIFFTEST_REF_KVM, count == 1, 1)) {
    rtl_xor(s, s0, s1, ddest);
    rtl_msb(s, s0, s0, width);
    rtl_set_OF(s, s0);
  }

  rtl_update_ZFSF(s, ddest, width);
#else
  rtl_shr(s, ddest, ddest, dsrc1);
  rtl_update_ZFSF(s, ddest, width);
#endif
#ifdef LAZY_CC
  //panic("TODO: implement CF and OF with lazy cc");
#endif
}

def_REHelper(sar) {
  // if ddest == dsrc1, rtl_sar() still only use the
  // lower 5 bits of dsrc1, which do not change after
  // rtl_sext(), and it is still sematically correct
  rtl_sext(s, ddest, ddest, width);
#ifndef __PA__
#ifdef CONFIG_ENGINE_INTERPRETER
  int count = *dsrc1 & 0x1f;
  if (count == 0) return;
#endif

  rtl_subi(s, s0, dsrc1, 1);
  rtl_sar(s, s1, ddest, s0); // shift (cnt - 1)
  rtl_andi(s, s0, s1, 0x1);
  rtl_set_CF(s, s0);
  rtl_sar(s, ddest, ddest, dsrc1);

  if (MUXDEF(CONFIG_DIFFTEST_REF_KVM, count == 1, 1)) {
    rtl_xor(s, s0, s1, ddest);
    rtl_msb(s, s0, s0, width);
    rtl_set_OF(s, s0);
  }

  rtl_update_ZFSF(s, ddest, width);
#else
  rtl_sar(s, ddest, ddest, dsrc1);
  rtl_update_ZFSF(s, ddest, width);
#endif
#ifdef LAZY_CC
  //panic("TODO: implement CF and OF with lazy cc");
#endif
}

def_REHelper(rol) {
  rtl_shl(s, s0, ddest, dsrc1);
  rtl_li(s, s1, width * 8);
  rtl_sub(s, s1, s1, dsrc1);
  rtl_shr(s, s1, ddest, s1);
  rtl_or(s, ddest, s0, s1);
  // unnecessary to update eflags in NEMU
  //difftest_skip_eflags(EFLAGS_MASK_ALL);
}
