#include "cc.h"

static inline def_EHelper(jmp) {
  // the target address is calculated at the decode stage
  rtl_j(s, s->jmp_pc);

  print_asm("jmp %x", s->jmp_pc);
}

static inline def_EHelper(jcc) {
  // the target address is calculated at the decode stage
  uint32_t cc = s->opcode & 0xf;
#ifdef LAZY_CC
  rtl_lazy_jcc(s, cc);
#else
  rtl_setcc(s, s0, cc);
  rtl_jrelop(s, RELOP_NE, s0, rz, s->jmp_pc);
#endif

  print_asm("j%s %x", get_cc_name(cc), s->jmp_pc);
}

static inline def_EHelper(jmp_rm) {
  rtl_jr(s, ddest);

  print_asm("jmp *%s", id_dest->str);
}

static inline def_EHelper(call) {
  // the target address is calculated at the decode stage
#ifdef __ICS_EXPORT
  TODO();
#else
  rtl_li(s, s0, s->seq_pc);
  rtl_push(s, s0);
  rtl_j(s, s->jmp_pc);
#endif

  print_asm("call %x", s->jmp_pc);
}

static inline def_EHelper(ret) {
#ifdef __ICS_EXPORT
  TODO();
#else
  rtl_pop(s, s0);
  rtl_jr(s, s0);
#endif

  print_asm("ret");
}

static inline def_EHelper(ret_imm) {
#ifdef __ICS_EXPORT
  TODO();
#else
  rtl_pop(s, s0);
  rtl_jr(s, s0);
  rtl_add(s, &cpu.esp, &cpu.esp, ddest);
#endif

  print_asm("ret %s", id_dest->str);
}

static inline def_EHelper(call_rm) {
#ifdef __ICS_EXPORT
  TODO();
#else
  rtl_li(s, s0, s->seq_pc);
  rtl_push(s, s0);
  rtl_jr(s, ddest);
#endif

  print_asm("call *%s", id_dest->str);
}
