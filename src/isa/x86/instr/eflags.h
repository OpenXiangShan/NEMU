def_EHelper(pushf) {
#ifdef CONFIG_x86_CC_LAZY
  rtl_lazy_setcc(s, &id_dest->val, CC_O);

  rtl_slli(s, &id_dest->val, &id_dest->val, 4);
  rtl_lazy_setcc(s, &id_src1->val, CC_S);
  rtl_or(s, &id_dest->val, &id_dest->val, &id_src1->val);

  rtl_slli(s, &id_dest->val, &id_dest->val, 1);
  rtl_lazy_setcc(s, &id_src1->val, CC_E);
  rtl_or(s, &id_dest->val, &id_dest->val, &id_src1->val);

  rtl_slli(s, &id_dest->val, &id_dest->val, 4);
  rtl_lazy_setcc(s, &id_src1->val, CC_P);
  rtl_or(s, &id_dest->val, &id_dest->val, &id_src1->val);

  rtl_slli(s, &id_dest->val, &id_dest->val, 2);
  rtl_lazy_setcc(s, &id_src1->val, CC_B);
  rtl_or(s, &id_dest->val, &id_dest->val, &id_src1->val);

  rtl_ori(s, &id_dest->val, &id_dest->val, 0x2);
  rtl_push(s, &id_dest->val);
#else
  void rtl_compute_eflags(Decode *s, rtlreg_t *dest);
  rtl_compute_eflags(s, s0);
  rtl_push(s, s0);
#endif

#ifdef CONFIG_DIFFTEST_REF_KVM
  extern void difftest_fix_eflags(void *arg);
  difftest_set_patch(difftest_fix_eflags, (void *)(uintptr_t)cpu.esp);
#endif
}

def_EHelper(clc) {
  IFNDEF(CONFIG_ENGINE_INTERPRETER, assert(0));
  IFDEF(CONFIG_x86_CC_LAZY, assert(0));
  rtl_set_CF(s, rz);
}

def_EHelper(stc) {
  IFNDEF(CONFIG_ENGINE_INTERPRETER, assert(0));
  IFDEF(CONFIG_x86_CC_LAZY, assert(0));
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
  rtl_pop(s, s0);
#ifdef CONFIG_x86_CC_LAZY
  rtl_set_lazycc(s, s0, NULL, NULL, LAZYCC_POPF, 0);
#else
  void rtl_set_eflags(Decode *s, const rtlreg_t *src);
  rtl_set_eflags(s, s0);
#endif
}

def_EHelper(sahf) {
#if CONFIG_x86_CC_LAZY
  rtl_lazy_setcc(s, s0, CC_O);
  rtl_lr(s, s1, R_AH, 1);
  rtl_or(s, s0, s0, s1);
  rtl_set_lazycc(s, s0, NULL, NULL, LAZYCC_POPF, 0);
#else
  void rtl_set_eflags(Decode *s, const rtlreg_t *src);
  rtl_slli(s, s0, &cpu.OF, 11);
  rtl_lr(s, s1, R_AH, 1);
  rtl_or(s, s0, s0, s1);
  rtl_set_eflags(s, s0);
#endif
}
