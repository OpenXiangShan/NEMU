#define get_shift(width) ((width) / 2)

def_EHelper(movs) {
  rtl_lm(s, s0, &cpu.esi, 0, s->isa.width, MMU_DYNAMIC);
  rtl_sm(s, s0, &cpu.edi, 0, s->isa.width, MMU_DYNAMIC);
  rtl_host_lm(s, s0, &cpu.DF, 4); // encoded value
  if (s->isa.width > 1) {
    rtl_slli(s, s0, s0, get_shift(s->isa.width));
  }
  rtl_add(s, &cpu.esi, &cpu.esi, s0);
  rtl_add(s, &cpu.edi, &cpu.edi, s0);
}

def_EHelper(rep_movs) {
  if (cpu.ecx != 0) {
    rtl_lm(s, s0, &cpu.esi, 0, s->isa.width, MMU_DYNAMIC);
    rtl_sm(s, s0, &cpu.edi, 0, s->isa.width, MMU_DYNAMIC);
    rtl_host_lm(s, s0, &cpu.DF, 4); // encoded value
    rtl_slli(s, s0, s0, get_shift(s->isa.width));
    rtl_add(s, &cpu.esi, &cpu.esi, s0);
    rtl_add(s, &cpu.edi, &cpu.edi, s0);

    rtl_subi(s, &cpu.ecx, &cpu.ecx, 1);
  }

  rtl_jrelop(s, RELOP_NE, &cpu.ecx, rz, s->pc);
}

def_EHelper(stos) {
  rt_decode_reg(s, id_src1, true, s->isa.width);
  rtl_sm(s, dsrc1, &cpu.edi, 0, s->isa.width, MMU_DYNAMIC);
  rtl_host_lm(s, s0, &cpu.DF, 4); // encoded value
  if (s->isa.width > 1) {
    rtl_slli(s, s0, s0, get_shift(s->isa.width));
  }
  rtl_add(s, &cpu.edi, &cpu.edi, s0);
}

def_EHelper(rep_stos) {
  if (cpu.ecx != 0) {
    rt_decode_reg(s, id_src1, true, s->isa.width);
    rtl_sm(s, dsrc1, &cpu.edi, 0, s->isa.width, MMU_DYNAMIC);

    rtl_host_lm(s, s0, &cpu.DF, 4); // encoded value
    rtl_slli(s, s0, s0, get_shift(s->isa.width));
    rtl_add(s, &cpu.edi, &cpu.edi, s0);

    rtl_subi(s, &cpu.ecx, &cpu.ecx, 1);
  }

  rtl_jrelop(s, RELOP_NE, &cpu.ecx, rz, s->pc);
}

def_EHelper(repz_cmps) {
  if (cpu.ecx != 0) {
    rtl_lm(s, &id_dest->val, &cpu.edi, 0, s->isa.width, MMU_DYNAMIC);
    rtl_lm(s, &id_src1->val, &cpu.esi, 0, s->isa.width, MMU_DYNAMIC);
    rtl_host_lm(s, s0, &cpu.DF, 4); // encoded value
    rtl_slli(s, s0, s0, get_shift(s->isa.width));
    rtl_add(s, &cpu.esi, &cpu.esi, s0);
    rtl_add(s, &cpu.edi, &cpu.edi, s0);
    rtl_subi(s, &cpu.ecx, &cpu.ecx, 1);
  }

//  if ((cpu.ecx != 0 && cpu.ZF)) rtl_j(s, cpu.pc);
  rtl_setrelop(s, RELOP_NE, s0, &cpu.ecx, rz);
  rtl_setrelop(s, RELOP_EQ, s1, &id_src1->val, &id_dest->val);
  rtl_and(s, s0, s0, s1);
  if (!*s0) {
    // exit the loop
#ifdef CONFIG_x86_CC_LAZY
    if (s->isa.flag_def != 0) {
      rtl_set_lazycc(s, &id_src1->val, &id_dest->val, NULL, LAZYCC_SUB, s->isa.width);
    }
#else
    int need_update_eflags = MUXDEF(CONFIG_x86_CC_SKIP, s->isa.flag_def != 0, true);
    if (need_update_eflags) {
      rtl_is_sub_carry(s, s1, &id_src1->val, &id_dest->val);
      rtl_set_CF(s, s1);
      rtl_sub(s, s1, &id_src1->val, &id_dest->val);
      rtl_update_ZFSF(s, s1, s->isa.width);
      rtl_is_sub_overflow(s, s1, s1, &id_src1->val, &id_dest->val, s->isa.width);
      rtl_set_OF(s, s1);
    }
#endif
  }
  rtl_jrelop(s, RELOP_NE, s0, rz, s->pc);
}

def_EHelper(repnz_scas) {
  if (cpu.ecx != 0) {
    rt_decode_reg(s, id_src1, true, s->isa.width);
    rtl_lm(s, s0, &cpu.edi, 0, s->isa.width, MMU_DYNAMIC);
    rtl_host_lm(s, s1, &cpu.DF, 4); // encoded value
    rtl_slli(s, s1, s1, get_shift(s->isa.width));
    rtl_add(s, &cpu.edi, &cpu.edi, s1);

    rtl_sub(s, s1, dsrc1, s0);
    rtl_update_ZFSF(s, s1, s->isa.width);
    rtl_is_sub_carry(s, s1, dsrc1, s0);
    rtl_set_CF(s, s1);
    rtl_sub(s, s1, dsrc1, s0);
    rtl_is_sub_overflow(s, s1, s1, dsrc1, s0, s->isa.width);
    rtl_set_OF(s, s1);

    rtl_subi(s, &cpu.ecx, &cpu.ecx, 1);
  }

  // if (!(cpu.ecx == 0 || cpu.ZF)) rtl_j(s, cpu.pc);
  rtl_setrelop(s, RELOP_EQ, s0, &cpu.ecx, rz);
  rtl_or(s, s0, s0, &cpu.ZF);
  rtl_jrelop(s, RELOP_EQ, s0, rz, s->pc);
}

#if 0
static inline def_EHelper(lods) {
  rtl_lm(s, ddest, &cpu.esi, 0, id_dest->width);
  rtl_addi(s, &cpu.esi, &cpu.esi, (cpu.DF ? -1 : 1) * id_dest->width);
  operand_write(s, id_dest, ddest);
}

static inline def_EHelper(stos) {
#ifndef CONFIG_ENGINE_INTERPRETER
  Assert(s->isa.rep_flags == 0, "not support REP in engines other than interpreter");
#endif

  word_t count = (s->isa.rep_flags ? cpu.ecx : 1);
  if (count != 0) {
    rtl_sm(s, &cpu.edi, 0, dsrc1, id_dest->width);
    return_on_mem_ex();
    rtl_addi(s, &cpu.edi, &cpu.edi, (cpu.DF ? -1 : 1) * id_dest->width);
  }
  if (s->isa.rep_flags && count != 0) {
    cpu.ecx --;
    if (count - 1 != 0) rtl_j(s, cpu.pc);
  }
}

static inline def_EHelper(scas) {
#ifndef CONFIG_ENGINE_INTERPRETER
  Assert(s->isa.rep_flags == 0, "not support REP in engines other than interpreter");
#endif

  int is_repnz = (s->isa.rep_flags == PREFIX_REPNZ);
  word_t count = (s->isa.rep_flags ? cpu.ecx : 1);
  if (count != 0) {
    rtl_lm(s, s0, &cpu.edi, 0, id_dest->width);
    return_on_mem_ex();
    rtl_setrelop(s, RELOP_EQ, s1, s0, dsrc1);
    rtl_set_ZF(s, s1);
    rtl_addi(s, &cpu.edi, &cpu.edi, (cpu.DF ? -1 : 1) * id_dest->width);
  }
  if (s->isa.rep_flags && count != 0) {
    cpu.ecx --;
    if ((count - 1 != 0) && (is_repnz ^ cpu.ZF)) rtl_j(s, cpu.pc);
  } else {
    rtl_sub(s, s1, dsrc1, s0);
    rtl_update_ZFSF(s, s1, id_dest->width);
    rtl_is_sub_carry(s, s1, dsrc1, s0);
    rtl_set_CF(s, s1);
    rtl_sub(s, s1, dsrc1, s0);
    rtl_is_sub_overflow(s, s1, s1, dsrc1, s0, id_dest->width);
    rtl_set_OF(s, s1);
  }
}

static inline def_EHelper(cmps) {
#ifndef CONFIG_ENGINE_INTERPRETER
  Assert(s->isa.rep_flags == 0, "not support REP in engines other than interpreter");
#endif

  int is_repnz = (s->isa.rep_flags == PREFIX_REPNZ);
  word_t count = (s->isa.rep_flags ? cpu.ecx : 1);
  if (count != 0) {
    rtl_lm(s, &id_dest->val, &cpu.edi, 0, id_dest->width);
    return_on_mem_ex();
    rtl_lm(s, &id_src1->val, &cpu.esi, 0, id_dest->width);
    return_on_mem_ex();
    rtl_setrelop(s, RELOP_EQ, s0, &id_dest->val, &id_src1->val);
    rtl_set_ZF(s, s0);
    rtl_addi(s, &cpu.esi, &cpu.esi, (cpu.DF ? -1 : 1) * id_dest->width);
    rtl_addi(s, &cpu.edi, &cpu.edi, (cpu.DF ? -1 : 1) * id_dest->width);
  }
  if (s->isa.rep_flags && count != 0) {
    cpu.ecx --;
    if ((count - 1 != 0) && (is_repnz ^ cpu.ZF)) rtl_j(s, cpu.pc);
    else {
      rtl_sub(s, s0, &id_src1->val, &id_dest->val);
      rtl_update_ZFSF(s, s0, id_dest->width);
      rtl_is_sub_carry(s, s1, &id_src1->val, &id_dest->val);
      rtl_set_CF(s, s1);
      rtl_is_sub_overflow(s, s0, s0, &id_src1->val, &id_dest->val, id_dest->width);
      rtl_set_OF(s, s0);
    }
  }
}
#endif
