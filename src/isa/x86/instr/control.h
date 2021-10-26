def_EHelper(call) {
  IFDEF(CONFIG_x86_CC_LAZY, clean_lazycc());
  // the target address is calculated at the decode stage
  rtl_li(s, s0, s->snpc);
  rtl_push(s, s0);
  ftrace_call(s->pc, id_dest->imm);
  rtl_j(s, id_dest->imm);
}

def_EHelper(ret) {
  IFDEF(CONFIG_x86_CC_LAZY, clean_lazycc());
  rtl_pop(s, s0);
  ftrace_ret(s->pc);
  rtl_jr(s, s0);
}

def_EHelper(jmp) {
  // the target address is calculated at the decode stage
  IFDEF(CONFIG_x86_CC_LAZY, clean_lazycc());
  rtl_j(s, id_dest->imm);
}

def_EHelper(jcc) {
  // the target address is calculated at the decode stage
  uint32_t cc = s->isa.opcode & 0xf;
#ifdef CONFIG_x86_CC_LAZY
  CCop op;
  op.target = id_dest->imm;
  rtl_lazy_jcc(s, &op, cc);
  rtl_jrelop(s, op.relop, op.src1, op.src2, op.target);
#else
  rtl_setcc(s, s0, cc);
  rtl_jrelop(s, RELOP_NE, s0, rz, id_dest->simm);
#endif
}

def_EHelper(call_E) {
  IFDEF(CONFIG_x86_CC_LAZY, clean_lazycc());
  rtl_decode_unary(s, true);
  rtl_li(s, s0, s->snpc);
  rtl_push(s, s0);
  ftrace_call(s->pc, *ddest);
  rtl_jr(s, ddest);
}

def_EHelper(jmp_E) {
  rtl_decode_unary(s, true);
  rtl_jr(s, ddest);
}

def_EHelper(ret_imm) {
  IFDEF(CONFIG_x86_CC_LAZY, clean_lazycc());
  rtl_pop(s, s0);
  rtl_addi(s, &cpu.esp, &cpu.esp, id_dest->val);
  ftrace_ret(s->pc);
  rtl_jr(s, s0);
}

def_EHelper(jecxz) {
  rtl_jrelop(s, RELOP_EQ, &cpu.ecx, rz, id_dest->simm);
}

#if 0
#ifndef __ICS_EXPORT

static inline def_EHelper(ljmp) {
  rtl_j(s, s->jmp_pc);
  rtl_li(s, s0, id_src1->imm);
  rtl_hostcall(s, HOSTCALL_CSR, NULL, s0, CSR_CS);
}

#else
static inline def_EHelper(call) {
  // the target address is calculated at the decode stage
  TODO();
}

static inline def_EHelper(ret) {
  TODO();
}

static inline def_EHelper(ret_imm) {
  TODO();
}

static inline def_EHelper(call_rm) {
  TODO();
}
#endif
#endif
