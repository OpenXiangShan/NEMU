static inline def_EHelper(movs) {
#ifndef __ENGINE_interpreter__
  panic("not support in engines other than interpreter");
#endif

  int count = (s->isa.rep_flags ? cpu.ecx : 1);
  while (count --) {
    rtl_lm(s, s0, &cpu.esi, 0, id_dest->width);
    rtl_sm(s, &cpu.edi, 0, s0, id_dest->width);

    rtl_addi(s, &cpu.esi, &cpu.esi, id_dest->width);
    rtl_addi(s, &cpu.edi, &cpu.edi, id_dest->width);
  }
  if (s->isa.rep_flags) cpu.ecx = 0;

  print_asm("movs (%%esi), (%%edi)");
}

static inline def_EHelper(lods) {
  rtl_lm(s, ddest, &cpu.esi, 0, id_dest->width);
  rtl_addi(s, &cpu.esi, &cpu.esi, id_dest->width);
  operand_write(s, id_dest, ddest);

  print_asm("lods (%%esi), %%eax");
}

static inline def_EHelper(stos) {
#ifndef __ENGINE_interpreter__
  panic("not support in engines other than interpreter");
#endif

  int count = (s->isa.rep_flags ? cpu.ecx : 1);
  while (count --) {
    rtl_sm(s, &cpu.edi, 0, dsrc1, id_dest->width);
    rtl_addi(s, &cpu.edi, &cpu.edi, id_dest->width);
  }
  if (s->isa.rep_flags) cpu.ecx = 0;

  print_asm("stos %%eax, (%%edi)");
}

static inline def_EHelper(scas) {
#ifndef __ENGINE_interpreter__
  panic("not support in engines other than interpreter");
#endif

  int count = (s->isa.rep_flags ? cpu.ecx : 1);
  int is_repnz = (s->isa.rep_flags == PREFIX_REPNZ);
  while (count --) {
    rtl_lm(s, s0, &cpu.edi, 0, id_dest->width);
    rtl_setrelop(s, RELOP_EQ, s0, s0, dsrc1);
    rtl_set_ZF(s, s0);
    rtl_addi(s, &cpu.edi, &cpu.edi, id_dest->width);
    if (s->isa.rep_flags) cpu.ecx --;
    if (is_repnz ^ cpu.ZF) break;
  }

  print_asm("stos %%eax, (%%edi)");
}

static inline def_EHelper(cmps) {
#ifndef __ENGINE_interpreter__
  panic("not support in engines other than interpreter");
#endif

  int count = (s->isa.rep_flags ? cpu.ecx : 1);
  int is_repnz = (s->isa.rep_flags == PREFIX_REPNZ);
  while (count --) {
    rtl_lm(s, s0, &cpu.edi, 0, id_dest->width);
    rtl_lm(s, s1, &cpu.esi, 0, id_dest->width);
    rtl_setrelop(s, RELOP_EQ, s0, s0, s1);
    rtl_set_ZF(s, s0);
    rtl_addi(s, &cpu.esi, &cpu.esi, id_dest->width);
    rtl_addi(s, &cpu.edi, &cpu.edi, id_dest->width);
    if (s->isa.rep_flags) cpu.ecx --;
    if (is_repnz ^ cpu.ZF) break;
  }

  print_asm("cmps (%%edi), (%%esi)");
}
