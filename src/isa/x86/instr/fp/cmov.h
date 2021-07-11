def_EHelper(fcmovb) {
#ifdef LAZY_CC
  rtl_lazy_setcc(s, s0, CC_B);
#else
  rtl_setcc(s, s0, CC_B);
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
