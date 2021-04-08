def_REHelper(movs) {
#ifndef CONFIG_ENGINE_INTERPRETER
  Assert(s->isa.rep_flags == 0, "not support REP in engines other than interpreter");
#endif

  word_t count = (s->isa.rep_flags ? cpu.ecx : 1);
  if (count != 0) {
    rtl_lm(s, s0, &cpu.esi, 0, width, MMU_DYNAMIC);
    rtl_sm(s, s0, &cpu.edi, 0, width, MMU_DYNAMIC);
    rtl_addi(s, &cpu.esi, &cpu.esi, (cpu.DF ? -1 : 1) * width);
    rtl_addi(s, &cpu.edi, &cpu.edi, (cpu.DF ? -1 : 1) * width);
  }
  if (s->isa.rep_flags && count != 0) {
    cpu.ecx --;
    if (count - 1 != 0) rtl_j(s, cpu.pc);
  }
}
