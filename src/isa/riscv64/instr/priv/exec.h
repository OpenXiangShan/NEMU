#define def_SYS_EHelper(name) \
def_EHelper(name) { \
  extern int rtl_sys_slow_path(Decode *s, rtlreg_t *dest, const rtlreg_t *src1, uint32_t id, rtlreg_t *jpc); \
  int jmp = rtl_sys_slow_path(s, ddest, dsrc1, id_src2->imm, s0); \
  if (jmp) rtl_priv_jr(s, s0); \
  else rtl_priv_next(s); \
}

#ifdef CONFIG_DEBUG
#define SYS_INSTR_LIST(f) \
  f(csrrw)  f(csrrs)  f(csrrc) f(csrrwi) f(csrrsi) f(csrrci) \
  f(ecall)  f(mret)   f(sret)  f(sfence_vma) f(wfi) \

MAP(SYS_INSTR_LIST, def_SYS_EHelper)
#else
def_SYS_EHelper(system)
#endif
