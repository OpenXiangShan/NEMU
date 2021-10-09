def_EHelper(pushf) {
  void rtl_compute_eflags(Decode *s, rtlreg_t *dest);
  rtl_compute_eflags(s, s0);
  rtl_push(s, s0);

  extern void difftest_fix_eflags(void *arg);
  difftest_set_patch(difftest_fix_eflags, (void *)(uintptr_t)cpu.esp);
}

def_EHelper(clc) {
  rtl_set_CF(s, rz);
}

def_EHelper(stc) {
  rtl_li(s, s0, 1);
  rtl_set_CF(s, s0);
}

def_EHelper(cld) {
  rtl_li(s, s0, 1); // encoded value
  rtl_host_sm(s, &cpu.DF, s0, 4);
}

def_EHelper(std) {
  IFNDEF(CONFIG_ENGINE_INTERPRETER, assert(0));
  rtl_li(s, s0, -1);
  rtl_host_sm(s, &cpu.DF, s0, 4);
//  rtl_set_DF(s, s0);
}

#if 0
static inline def_EHelper(cli) {
  rtl_set_IF(s, rz);
}

static inline def_EHelper(sti) {
  rtl_li(s, s0, 1);
  rtl_set_IF(s, s0);
}
#endif

def_EHelper(popf) {
  void rtl_set_eflags(Decode *s, const rtlreg_t *src);
  rtl_pop(s, s0);
  rtl_set_eflags(s, s0);
}

def_EHelper(sahf) {
  void rtl_set_eflags(Decode *s, const rtlreg_t *src);
  void rtl_compute_eflags(Decode *s, rtlreg_t *dest);

  rtl_compute_eflags(s, s0);
  rtl_andi(s, s0, s0, ~0xff);
  rtl_lr(s, s1, R_AH, 1);
  rtl_or(s, s0, s0, s1);
  rtl_set_eflags(s, s0);
}
