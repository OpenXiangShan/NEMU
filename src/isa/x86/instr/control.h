def_EHelper(call) {
  IFDEF(LAZY_CC, clean_lazycc());
  // the target address is calculated at the decode stage
  rtl_push(s, dsrc1);
  rtl_j(s, id_dest->imm);
}

def_EHelper(ret) {
  IFDEF(LAZY_CC, clean_lazycc());
  rtl_pop(s, s0);
  rtl_jr(s, s0);
}

def_EHelper(jmp) {
  // the target address is calculated at the decode stage
  IFDEF(LAZY_CC, clean_lazycc());
  rtl_j(s, id_dest->imm);
}

def_EHelper(jcc) {
  // the target address is calculated at the decode stage
  uint32_t cc = s->isa.opcode & 0xf;
#ifdef LAZY_CC
  rtl_lazy_setcc(s, s2, cc);
  rtl_mv(s, s0, s2);
  clean_lazycc();
#else
  rtl_setcc(s, s0, cc);
#endif
  rtl_jrelop(s, RELOP_NE, s0, rz, id_dest->simm);
}

def_EHelper(call_E) {
  IFDEF(LAZY_CC, clean_lazycc(s));
  rt_decode_E(s, 4);
  rtl_li(s, s0, s->snpc);
  rtl_push(s, s0);
  rtl_jr(s, ddest);
}

def_EHelper(jmp_E) {
  IFDEF(LAZY_CC, clean_lazycc(s));
  rt_decode_E(s, 4);
  rtl_jr(s, ddest);
}


#if 0
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
