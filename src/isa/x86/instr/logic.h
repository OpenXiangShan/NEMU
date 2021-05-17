def_EHelper(and) {
  rtl_decode_binary(s, true, true);
  rtl_and(s, ddest, ddest, dsrc1);
#ifdef CONFIG_x86_CC_LAZY
  rtl_set_lazycc(s, ddest, NULL, NULL, LAZYCC_LOGIC, s->isa.width);
#else
  int need_update_eflags = MUXDEF(CONFIG_x86_CC_SKIP, s->isa.flag_def != 0, true);
  if (need_update_eflags) {
    rtl_update_ZFSF(s, ddest, s->isa.width);
    rtl_mv(s, &cpu.CF, rz);
    rtl_mv(s, &cpu.OF, rz);
  }
#endif
  rtl_wb(s, ddest);
}

def_EHelper(or) {
  rtl_decode_binary(s, true, true);
  rtl_or(s, ddest, ddest, dsrc1);
#ifdef CONFIG_x86_CC_LAZY
  rtl_set_lazycc(s, ddest, NULL, NULL, LAZYCC_LOGIC, s->isa.width);
#else
  int need_update_eflags = MUXDEF(CONFIG_x86_CC_SKIP, s->isa.flag_def != 0, true);
  if (need_update_eflags) {
    rtl_update_ZFSF(s, ddest, s->isa.width);
    rtl_mv(s, &cpu.CF, rz);
    rtl_mv(s, &cpu.OF, rz);
  }
#endif
  rtl_wb(s, ddest);
}

def_EHelper(test) {
  rtl_decode_binary(s, true, true);
  rtl_and(s, s0, ddest, dsrc1);
#ifdef CONFIG_x86_CC_LAZY
  rtl_set_lazycc(s, s0, NULL, NULL, LAZYCC_LOGIC, s->isa.width);
#else
  int need_update_eflags = MUXDEF(CONFIG_x86_CC_SKIP, s->isa.flag_def != 0, true);
  if (need_update_eflags) {
    rtl_update_ZFSF(s, s0, s->isa.width);
    rtl_mv(s, &cpu.CF, rz);
    rtl_mv(s, &cpu.OF, rz);
  }
#endif
}

def_EHelper(xor) {
  rtl_decode_binary(s, true, true);
  rtl_xor(s, ddest, ddest, dsrc1);
#ifdef CONFIG_x86_CC_LAZY
  rtl_set_lazycc(s, ddest, NULL, NULL, LAZYCC_LOGIC, s->isa.width);
#else
  int need_update_eflags = MUXDEF(CONFIG_x86_CC_SKIP, s->isa.flag_def != 0, true);
  if (need_update_eflags) {
    rtl_update_ZFSF(s, ddest, s->isa.width);
    rtl_mv(s, &cpu.CF, rz);
    rtl_mv(s, &cpu.OF, rz);
  }
#endif
  rtl_wb(s, ddest);
}

def_EHelper(not) {
  rtl_decode_unary(s, true);
  rtl_not(s, ddest, ddest);
  rtl_wb(s, ddest);
}

def_EHelper(setcc) {
  rtl_decode_unary(s, false);
  uint32_t cc = s->isa.opcode & 0xf;
#ifdef CONFIG_x86_CC_LAZY
  rtl_lazy_setcc(s, ddest, cc);
#else
  rtl_setcc(s, ddest, cc);
#endif
  rtl_wb(s, ddest);
}

def_EHelper(shl) {
  rtl_decode_binary(s, true, true);
#ifndef CONFIG_PA
#ifdef CONFIG_ENGINE_INTERPRETER
//  int count = *dsrc1 & 0x1f;
//  if (count == 0) return;
#endif
  int need_update_eflags = MUXDEF(CONFIG_x86_CC_SKIP, s->isa.flag_def != 0, true);
  if (need_update_eflags) {
    rtl_subi(s, s0, dsrc1, 1);
    rtl_shl(s, s1, ddest, s0); // shift (cnt - 1)
    rtl_msb(s, s0, s1, s->isa.width);
    rtl_set_CF(s, s0);
    rtl_shl(s, ddest, ddest, dsrc1);

    if (MUXDEF(CONFIG_DIFFTEST_REF_KVM, count == 1, 1)) {
      rtl_xor(s, s0, s1, ddest);
      rtl_msb(s, s0, s0, s->isa.width);
      rtl_set_OF(s, s0);
    }

    rtl_update_ZFSF(s, ddest, s->isa.width);
  } else {
    rtl_shl(s, ddest, ddest, dsrc1);
  }
#else
  rtl_shl(s, ddest, ddest, dsrc1);
  rtl_update_ZFSF(s, ddest, s->isa.width);
#endif
#ifdef CONFIG_x86_CC_LAZY
  //panic("TODO: implement CF and OF with lazy cc");
#endif
  rtl_wb(s, ddest);
}

def_EHelper(shr) {
  rtl_decode_binary(s, true, true);
#ifndef CONFIG_PA
#ifdef CONFIG_ENGINE_INTERPRETER
//  int count = *dsrc1 & 0x1f;
//  if (count == 0) return;
#endif
  int need_update_eflags = MUXDEF(CONFIG_x86_CC_SKIP, s->isa.flag_def != 0, true);
  if (need_update_eflags) {
    rtl_subi(s, s0, dsrc1, 1);
    rtl_shr(s, s1, ddest, s0); // shift (cnt - 1)
    rtl_andi(s, s0, s1, 0x1);
    rtl_set_CF(s, s0);
    rtl_shr(s, ddest, ddest, dsrc1);

    if (MUXDEF(CONFIG_DIFFTEST_REF_KVM, count == 1, 1)) {
      rtl_xor(s, s0, s1, ddest);
      rtl_msb(s, s0, s0, s->isa.width);
      rtl_set_OF(s, s0);
    }

    rtl_update_ZFSF(s, ddest, s->isa.width);
  } else {
    rtl_shr(s, ddest, ddest, dsrc1);
  }
#else
  rtl_shr(s, ddest, ddest, dsrc1);
  rtl_update_ZFSF(s, ddest, s->isa.width);
#endif
#ifdef CONFIG_x86_CC_LAZY
  //panic("TODO: implement CF and OF with lazy cc");
#endif
  rtl_wb(s, ddest);
}

def_EHelper(sar) {
  rtl_decode_binary(s, true, true);

  // if ddest == dsrc1, rtl_sar() still only use the
  // lower 5 bits of dsrc1, which do not change after
  // rtl_sext(), and it is still sematically correct
  rtl_sext(s, ddest, ddest, s->isa.width);
#ifndef CONFIG_PA
#ifdef CONFIG_ENGINE_INTERPRETER
//  int count = *dsrc1 & 0x1f;
//  if (count == 0) return;
#endif
  int need_update_eflags = MUXDEF(CONFIG_x86_CC_SKIP, s->isa.flag_def != 0, true);
  if (need_update_eflags) {
    rtl_subi(s, s0, dsrc1, 1);
    rtl_sar(s, s1, ddest, s0); // shift (cnt - 1)
    rtl_andi(s, s0, s1, 0x1);
    rtl_set_CF(s, s0);
    rtl_sar(s, ddest, ddest, dsrc1);

    if (MUXDEF(CONFIG_DIFFTEST_REF_KVM, count == 1, 1)) {
      rtl_xor(s, s0, s1, ddest);
      rtl_msb(s, s0, s0, s->isa.width);
      rtl_set_OF(s, s0);
    }

    rtl_update_ZFSF(s, ddest, s->isa.width);
  } else {
    rtl_sar(s, ddest, ddest, dsrc1);
  }
#else
  rtl_sar(s, ddest, ddest, dsrc1);
  rtl_update_ZFSF(s, ddest, s->isa.width);
#endif
#ifdef CONFIG_x86_CC_LAZY
  //panic("TODO: implement CF and OF with lazy cc");
#endif
  rtl_wb(s, ddest);
}

def_EHelper(rol) {
  rtl_decode_binary(s, true, true);
  rtl_shl(s, s0, ddest, dsrc1);
  rtl_li(s, s1, s->isa.width * 8);
  rtl_sub(s, s1, s1, dsrc1);
  rtl_shr(s, s1, ddest, s1);
  rtl_or(s, ddest, s0, s1);
  rtl_wb(s, ddest);
  // unnecessary to update eflags in NEMU
  //difftest_skip_eflags(EFLAGS_MASK_ALL);
}

def_EHelper(ror) {
  rtl_decode_binary(s, true, true);
  rtl_shr(s, s0, ddest, dsrc1);
  rtl_li(s, s1, s->isa.width * 8);
  rtl_sub(s, s1, s1, dsrc1);
  rtl_shl(s, s1, ddest, s1);
  rtl_or(s, ddest, s0, s1);
  rtl_wb(s, ddest);
  // unnecessary to update eflags in NEMU
  //difftest_skip_eflags(EFLAGS_MASK_ALL);
}

def_EHelper(shld) {
  assert(s->isa.width == 4);
  rtl_decode_binary(s, true, true);

  rtl_shl(s, s0, ddest, dsrc2);

  rtl_li(s, s1, 31);
  rtl_sub(s, s1, s1, dsrc2);
  // shift twice to deal with dsrc1 = 0
  // the first shift is still right even if we do not
  // mask out the high part of `dsrc1`, since we have
  //   (31 - (dsrc1 & 0x1f)) = (31 - dsrc1 % 32) = (31 - dsrc1) mod 32
  rtl_shr(s, s1, dsrc1, s1);
  rtl_shri(s, s1, s1, 1);

  rtl_or(s, ddest, s0, s1);
  rtl_wb(s, ddest);

#ifndef CONFIG_x86_CC_LAZY
  rtl_update_ZFSF(s, ddest, s->isa.width);
  // unnecessary to update CF and OF in NEMU
#endif
  print_asm_template3(shld);
}

def_EHelper(shrd) {
  assert(s->isa.width == 4);
  rtl_decode_binary(s, true, true);

#ifdef CONFIG_ENGINE_INTERPRETER
  int count = *dsrc2 & 0x1f;
  if (count == 0) {
    rtl_wb(s, ddest);
    assert(0);
    //return;
  }
#endif
  rtl_subi(s, s0, dsrc2, 1);
  rtl_shr(s, s1, ddest, s0); // shift (cnt - 1)
  rtl_andi(s, s0, s1, 0x1);
  rtl_set_CF(s, s0);
  rtl_shr(s, s0, ddest, dsrc2);

  rtl_li(s, s1, 31);
  rtl_sub(s, s1, s1, dsrc2);
  // shift twice to deal with dsrc1 = 0
  // the first shift is still right even if we do not
  // mask out the high part of `dsrc1`, since we have
  //   (31 - (dsrc1 & 0x1f)) = (31 - dsrc1 % 32) = (31 - dsrc1) mod 32
  rtl_shl(s, s1, dsrc1, s1);
  rtl_shli(s, s1, s1, 1);

  rtl_or(s, ddest, s0, s1);
  rtl_wb(s, ddest);

#ifndef CONFIG_x86_CC_LAZY
  rtl_update_ZFSF(s, ddest, s->isa.width);
  // unnecessary to update CF and OF in NEMU
#endif
}

#if 0
static inline def_EHelper(rcr) {
  rtl_shr(s, s0, ddest, dsrc1);

  rtl_get_CF(s, s1);
  rtl_shli(s, s1, s1, 31);
  rtl_shr(s, s1, s1, dsrc1);
  rtl_shli(s, s1, s1, 1);
  rtl_or(s, s0, s0, s1);

  rtl_li(s, s1, 1);
  rtl_shl(s, s1, s1, dsrc1);
  rtl_shri(s, s1, s1, 1);
  rtl_and(s, s1, ddest, s1);
  rtl_setrelopi(s, RELOP_NE, s1, s1, 0);
  rtl_set_CF(s, s1);

  rtl_li(s, s1, id_dest->s->isa.width * 8);
  rtl_sub(s, s1, s1, dsrc1);
  rtl_shl(s, s1, ddest, s1);
  rtl_shli(s, s1, s1, 1);
  rtl_or(s, ddest, s0, s1);

  operand_write(s, id_dest, ddest);
  print_asm_template2(rcr);
}

static inline def_EHelper(rcl) {
  rtl_shl(s, s0, ddest, dsrc1);

  rtl_get_CF(s, s1);
  rtl_shl(s, s1, s1, dsrc1);
  rtl_shri(s, s1, s1, 1);
  rtl_or(s, s0, s0, s1);

  rtl_li(s, s1, 0x80000000);
  rtl_shr(s, s1, s1, dsrc1);
  rtl_shli(s, s1, s1, 1);
  rtl_and(s, s1, ddest, s1);
  rtl_setrelopi(s, RELOP_NE, s1, s1, 0);
  rtl_set_CF(s, s1);

  rtl_li(s, s1, id_dest->s->isa.width * 8);
  rtl_sub(s, s1, s1, dsrc1);
  rtl_shr(s, s1, ddest, s1);
  rtl_shri(s, s1, s1, 1);
  rtl_or(s, ddest, s0, s1);

  operand_write(s, id_dest, ddest);
  print_asm_template2(rcl);
}
#endif
