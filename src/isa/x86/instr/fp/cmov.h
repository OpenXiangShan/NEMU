def_EHelper(fcmovb) {
#ifdef CONFIG_x86_CC_LAZY
  rtl_lazy_setcc(s, s0, CC_B);
#else
  rtl_setcc(s, s0, CC_B);
#endif
  if (*s0) { *dfdest = *dfsrc1; }
}

def_EHelper(fcmove) {
#ifdef CONFIG_x86_CC_LAZY
  rtl_lazy_setcc(s, s0, CC_E);
#else
  rtl_setcc(s, s0, CC_E);
#endif
  if (*s0) { *dfdest = *dfsrc1; }
}

def_EHelper(fcmovbe) {
#ifdef CONFIG_x86_CC_LAZY
  rtl_lazy_setcc(s, s0, CC_BE);
#else
  rtl_setcc(s, s0, CC_BE);
#endif
  if (*s0) { *dfdest = *dfsrc1; }
}

def_EHelper(fcmovu) {
#ifdef CONFIG_x86_CC_LAZY
  rtl_lazy_setcc(s, s0, CC_P);
#else
  rtl_setcc(s, s0, CC_P);
#endif
  if (*s0) { *dfdest = *dfsrc1; }
}

def_EHelper(fcmovnb) {
#ifdef CONFIG_x86_CC_LAZY
  rtl_lazy_setcc(s, s0, CC_B);
#else
  rtl_setcc(s, s0, CC_B);
#endif
  if (!*s0) { *dfdest = *dfsrc1; }
}

def_EHelper(fcmovne) {
#ifdef CONFIG_x86_CC_LAZY
  rtl_lazy_setcc(s, s0, CC_E);
#else
  rtl_setcc(s, s0, CC_E);
#endif
  if (!*s0) { *dfdest = *dfsrc1; }
}

def_EHelper(fcmovnbe) {
#ifdef CONFIG_x86_CC_LAZY
  rtl_lazy_setcc(s, s0, CC_BE);
#else
  rtl_setcc(s, s0, CC_BE);
#endif
  if (!*s0) { *dfdest = *dfsrc1; }
}

def_EHelper(fcmovnu) {
#ifdef CONFIG_x86_CC_LAZY
  rtl_lazy_setcc(s, s0, CC_P);
#else
  rtl_setcc(s, s0, CC_P);
#endif
  if (!*s0) { *dfdest = *dfsrc1; }
}

