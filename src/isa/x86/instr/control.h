def_EHelper(call) {
  IFDEF(LAZY_CC, clean_lazycc(s));
  // the target address is calculated at the decode stage
  rtl_push(s, dsrc1);
  rtl_j(s, id_dest->imm);
}

def_EHelper(ret) {
  IFDEF(LAZY_CC, clean_lazycc(s));
  rtl_pop(s, s0);
  rtl_jr(s, s0);
}


#if 0
static inline def_EHelper(jmp) {
  // the target address is calculated at the decode stage
  rtl_j(s, s->jmp_pc);
#ifdef LAZY_CC
  clean_lazycc(s);
#endif
  print_asm("jmp %x", s->jmp_pc);
}

static inline def_EHelper(jcc) {
  // the target address is calculated at the decode stage
  uint32_t cc = s->opcode & 0xf;
#ifdef LAZY_CC
  rtl_lazy_jcc(s, cc);
  clean_lazycc(s);
#else
  rtl_setcc(s, s0, cc);
  rtl_jrelop(s, RELOP_NE, s0, rz, s->jmp_pc);
#endif

  print_asm("j%s %x", get_cc_name(cc), s->jmp_pc);
}

static inline def_EHelper(jmp_rm) {
  rtl_jr(s, ddest);
#ifdef LAZY_CC
  clean_lazycc(s);
#endif
  print_asm("jmp *%s", id_dest->str);
}

#ifndef __ICS_EXPORT

static inline def_EHelper(ret_imm) {
  rtl_pop(s, s0);
  rtl_jr(s, s0);
#ifdef LAZY_CC
  clean_lazycc(s);
#endif
  rtl_add(s, &cpu.esp, &cpu.esp, ddest);
  print_asm("ret %s", id_dest->str);
}

static inline def_EHelper(call_rm) {
  rtl_li(s, s0, s->seq_pc);
  rtl_push(s, s0);
  rtl_jr(s, ddest);
#ifdef LAZY_CC
  clean_lazycc(s);
#endif
  print_asm("call *%s", id_dest->str);
}

static inline def_EHelper(ljmp) {
  rtl_j(s, s->jmp_pc);
  rtl_li(s, s0, id_src1->imm);
  rtl_hostcall(s, HOSTCALL_CSR, NULL, s0, CSR_CS);
  print_asm("ljmp %s,%s", id_src1->str, id_dest->str);
}

static inline def_EHelper(jecxz) {
  rtl_jrelop(s, RELOP_EQ, &cpu.ecx, rz, s->jmp_pc);
  print_asm("jecxz %x", s->jmp_pc);
}
#else
static inline def_EHelper(call) {
  // the target address is calculated at the decode stage
  TODO();
  print_asm("call %x", s->jmp_pc);
}

static inline def_EHelper(ret) {
  TODO();
  print_asm("ret");
}

static inline def_EHelper(ret_imm) {
  TODO();
  print_asm("ret %s", id_dest->str);
}

static inline def_EHelper(call_rm) {
  TODO();
  print_asm("call *%s", id_dest->str);
}
#endif
#endif
