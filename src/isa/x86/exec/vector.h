static inline int SSEprefix(Decode *s) {
  assert(!(s->isa.rep_flags != 0 && s->isa.is_operand_size_16));
  if (s->isa.is_operand_size_16) return 1;
  else if (s->isa.rep_flags == PREFIX_REP) return 2;
  else if (s->isa.rep_flags == PREFIX_REPNZ) return 3;
  else return 0;
}

static inline def_EHelper(movq_E2xmm) {
  int pfx = SSEprefix(s);
  if (pfx == 2) {
    rtl_lm(s, s0, s->isa.mbase, s->isa.moff + 0, 4);
    cpu.xmm[id_dest->reg]._32[0] = *s0;
    rtl_lm(s, s0, s->isa.mbase, s->isa.moff + 4, 4);
    cpu.xmm[id_dest->reg]._32[1] = *s0;
    print_asm_template2(movq);
  } else if (pfx == 1) {
    rtl_li(s, s0, cpu.xmm[id_dest->reg]._32[0]);
  operand_write(s, id_src1, s0);
    print_asm_template2(movd);
  } else {
    assert(0);
  }
}

static inline def_EHelper(movq_xmm2E) {
  assert(SSEprefix(s) == 1);
  *s0 = cpu.xmm[id_src1->reg]._32[0];
  rtl_sm(s, s->isa.mbase, s->isa.moff + 0, s0, 4);
  *s0 = cpu.xmm[id_src1->reg]._32[1];
  rtl_sm(s, s->isa.mbase, s->isa.moff + 4, s0, 4);
  print_asm_template2(movq);
}

static inline def_EHelper(movdqa_E2xmm) {
  assert(SSEprefix(s) == 1);
  if (id_src1->type == OP_TYPE_REG) {
    cpu.xmm[id_dest->reg] = cpu.xmm[id_src1->reg];
  } else {
    rtl_lm(s, s0, s->isa.mbase, s->isa.moff + 0, 4);
    cpu.xmm[id_dest->reg]._32[0] = *s0;
    rtl_lm(s, s0, s->isa.mbase, s->isa.moff + 4, 4);
    cpu.xmm[id_dest->reg]._32[1] = *s0;
    rtl_lm(s, s0, s->isa.mbase, s->isa.moff + 8, 4);
    cpu.xmm[id_dest->reg]._32[2] = *s0;
    rtl_lm(s, s0, s->isa.mbase, s->isa.moff + 12, 4);
    cpu.xmm[id_dest->reg]._32[3] = *s0;
  }
  print_asm_template2(movqda);
}

static inline def_EHelper(psrlq) {
  assert(SSEprefix(s) == 1);
  assert(id_dest->type == OP_TYPE_REG);
  assert(s->isa.ext_opcode == 2);
  cpu.xmm[id_dest->reg]._128 >>= id_src1->imm;
  print_asm_template2(psrlq);
}

static inline def_EHelper(pxor) {
  assert(SSEprefix(s) == 1);
  union {
    __uint128_t _128;
    uint32_t _32[4];
  } src;
  if (id_src1->type == OP_TYPE_REG) {
    src._128 = cpu.xmm[id_src1->reg]._128;
  } else {
    rtl_lm(s, s0, s->isa.mbase, s->isa.moff + 0, 4);
    src._32[0] = *s0;
    rtl_lm(s, s0, s->isa.mbase, s->isa.moff + 4, 4);
    src._32[1] = *s0;
    rtl_lm(s, s0, s->isa.mbase, s->isa.moff + 8, 4);
    src._32[2] = *s0;
    rtl_lm(s, s0, s->isa.mbase, s->isa.moff + 12, 4);
    src._32[3] = *s0;
  }
  cpu.xmm[id_dest->reg]._128 ^= src._128;
  print_asm_template2(pxor);
}
