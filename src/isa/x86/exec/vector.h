static inline def_EHelper(movq_E2xmm) {
  assert(s->isa.rep_flags == PREFIX_REP && !s->isa.is_operand_size_16);
  rtl_lm(s, s0, s->isa.mbase, s->isa.moff + 0, 4);
  rtl_lm(s, s1, s->isa.mbase, s->isa.moff + 4, 4);
  cpu.xmm[id_dest->reg][0] = *s0;
  cpu.xmm[id_dest->reg][1] = *s1;
  print_asm_template2(movq);
}

static inline def_EHelper(movq_xmm2E) {
  assert(s->isa.rep_flags == 0 && s->isa.is_operand_size_16);
  *s0 = cpu.xmm[id_src1->reg][0];
  *s1 = cpu.xmm[id_src1->reg][1];
  rtl_sm(s, s->isa.mbase, s->isa.moff + 0, s0, 4);
  rtl_sm(s, s->isa.mbase, s->isa.moff + 4, s1, 4);
  print_asm_template2(movq);
}
