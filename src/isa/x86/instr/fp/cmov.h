def_EHelper(fcmovb) {
#ifdef LAZY_CC
  rtl_lazy_setcc(s, s0, CC_B);
#else
  rtl_setcc(s, s0, CC_B);
#endif
  if (*s0) { *dfdest = *dfsrc1; }
}

def_EHelper(fcmove) {
#ifdef LAZY_CC
  rtl_lazy_setcc(s, s0, CC_E);
#else
  rtl_setcc(s, s0, CC_E);
#endif
  if (*s0) { *dfdest = *dfsrc1; }
}

def_EHelper(fcmovbe) {
#ifdef LAZY_CC
  rtl_lazy_setcc(s, s0, CC_BE);
#else
  rtl_setcc(s, s0, CC_BE);
#endif
  if (*s0) { *dfdest = *dfsrc1; }
}

def_EHelper(fcmovu) {
#ifdef LAZY_CC
  rtl_lazy_setcc(s, s0, CC_P);
#else
  rtl_setcc(s, s0, CC_P);
#endif
  if (*s0) { *dfdest = *dfsrc1; }
}

def_EHelper(fcmovnb) {
#ifdef LAZY_CC
  rtl_lazy_setcc(s, s0, CC_B);
#else
  rtl_setcc(s, s0, CC_B);
#endif
  if (!*s0) { *dfdest = *dfsrc1; }
}

def_EHelper(fcmovne) {
#ifdef LAZY_CC
  rtl_lazy_setcc(s, s0, CC_E);
#else
  rtl_setcc(s, s0, CC_E);
#endif
  if (!*s0) { *dfdest = *dfsrc1; }
}

def_EHelper(fcmovnbe) {
#ifdef LAZY_CC
  rtl_lazy_setcc(s, s0, CC_BE);
#else
  rtl_setcc(s, s0, CC_BE);
#endif
  if (!*s0) { *dfdest = *dfsrc1; }
}

def_EHelper(fcmovnu) {
#ifdef LAZY_CC
  rtl_lazy_setcc(s, s0, CC_P);
#else
  rtl_setcc(s, s0, CC_P);
#endif
  if (!*s0) { *dfdest = *dfsrc1; }
}

