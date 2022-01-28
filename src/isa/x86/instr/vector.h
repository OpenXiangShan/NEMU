def_EHelper(movq_E2xmm) {
  assert(0);
  rtl_decode_binary(s, false, false);

  rtl_lm(s, s0, &s->extraInfo->isa.mbr, s->extraInfo->isa.moff + 0, 4, MMU_DYNAMIC);
  cpu.xmm[id_dest->reg]._32[0] = *s0;
  rtl_lm(s, s0, &s->extraInfo->isa.mbr, s->extraInfo->isa.moff + 4, 4, MMU_DYNAMIC);
  cpu.xmm[id_dest->reg]._32[1] = *s0;

    //rtl_li(s, s0, cpu.xmm[id_dest->reg]._32[0]);
    //operand_write(s, id_src1, s0);
}

def_EHelper(movq_xmm2E) {
  assert(0);
  rtl_decode_binary(s, false, false);
  *s0 = cpu.xmm[id_src1->reg]._32[0];
  rtl_sm(s, s0, &s->extraInfo->isa.mbr, s->extraInfo->isa.moff + 0, 4, MMU_DYNAMIC);
  *s0 = cpu.xmm[id_src1->reg]._32[1];
  rtl_sm(s, s0, &s->extraInfo->isa.mbr, s->extraInfo->isa.moff + 4, 4, MMU_DYNAMIC);
}

def_EHelper(movdqa_E2xmm) {
  assert(0);
  rtl_decode_binary(s, false, false);
  if (id_src1->type == OP_TYPE_REG) {
    cpu.xmm[id_dest->reg] = cpu.xmm[id_src1->reg];
  } else {
    assert(0);
    rtl_lm(s, s0, &s->extraInfo->extraInfo->isa.mbr, s->extraInfo->isa.moff + 0, 4, MMU_DYNAMIC);
    cpu.xmm[id_dest->reg]._32[0] = *s0;
    rtl_lm(s, s0, &s->extraInfo->isa.mbr, s->extraInfo->isa.moff + 4, 4, MMU_DYNAMIC);
    cpu.xmm[id_dest->reg]._32[1] = *s0;
    rtl_lm(s, s0, &s->extraInfo->isa.mbr, s->extraInfo->isa.moff + 8, 4, MMU_DYNAMIC);
    cpu.xmm[id_dest->reg]._32[2] = *s0;
    rtl_lm(s, s0, &s->extraInfo->isa.mbr, s->extraInfo->isa.moff + 12, 4, MMU_DYNAMIC);
    cpu.xmm[id_dest->reg]._32[3] = *s0;
  }
}

def_EHelper(psrlq) {
  assert(0);
  rtl_decode_unary(s, false);
  assert(id_dest->type == OP_TYPE_REG);
#ifdef __LP64__
  cpu.xmm[id_dest->reg]._128 >>= id_src1->imm;
#else
  cpu.xmm[id_dest->reg]._128._64[0] >>= id_src1->imm;
  cpu.xmm[id_dest->reg]._128._64[1] >>= id_src1->imm;
#endif
}

def_EHelper(movd_xmm2E) {
  assert(0);
  rtl_decode_binary(s, false, false);
  rtl_li(s, s0, cpu.xmm[id_src1->reg]._32[0]);
  rtl_wb(s, s0);
}

def_EHelper(pxor) {
  assert(0);
  rtl_decode_binary(s, false, false);
  union {
    __uint128_t _128;
    uint32_t _32[4];
  } src;
  if (id_src1->type == OP_TYPE_REG) {
    assert(0);
    src._128 = cpu.xmm[id_src1->reg]._128;
  } else {
    rtl_lm(s, s0, &s->extraInfo->isa.mbr, s->extraInfo->isa.moff + 0, 4, MMU_DYNAMIC);
    src._32[0] = *s0;
    rtl_lm(s, s0, &s->extraInfo->isa.mbr, s->extraInfo->isa.moff + 4, 4, MMU_DYNAMIC);
    src._32[1] = *s0;
    rtl_lm(s, s0, &s->extraInfo->isa.mbr, s->extraInfo->isa.moff + 8, 4, MMU_DYNAMIC);
    src._32[2] = *s0;
    rtl_lm(s, s0, &s->extraInfo->isa.mbr, s->extraInfo->isa.moff + 12, 4, MMU_DYNAMIC);
    src._32[3] = *s0;
  }
#ifdef __LP64__
  cpu.xmm[id_dest->reg]._128 ^= src._128;
#else
  cpu.xmm[id_dest->reg]._128._64[0] ^= src._128._64[0];
  cpu.xmm[id_dest->reg]._128._64[1] ^= src._128._64[1];
#endif
}
