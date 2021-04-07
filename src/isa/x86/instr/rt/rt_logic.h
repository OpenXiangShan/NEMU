// dest <- and result
static inline void and_internal(Decode *s, int width) {
  rtl_and(s, s0, ddest, dsrc1);
  rtl_update_ZFSF(s, s0, width);
  rtl_mv(s, &cpu.CF, rz);
  rtl_mv(s, &cpu.OF, rz);
}

def_REHelper(and) {
#ifdef LAZY_CC
  rtl_and(s, ddest, ddest, dsrc1);
  rtl_set_lazycc(s, ddest, NULL, NULL, LAZYCC_LOGIC, id_dest->width);
#else
  and_internal(s, width);
  rtl_mv(s, ddest, s0);
#endif
}

def_REHelper(or) {
  rtl_or(s, ddest, ddest, dsrc1);
#ifdef LAZY_CC
  rtl_set_lazycc(s, ddest, NULL, NULL, LAZYCC_LOGIC, width);
#else
  rtl_update_ZFSF(s, ddest, width);
  rtl_mv(s, &cpu.CF, rz);
  rtl_mv(s, &cpu.OF, rz);
#endif
}


def_REHelper(test) {
#ifdef LAZY_CC
  rtl_and(s, s0, ddest, dsrc1);
  rtl_set_lazycc(s, s0, NULL, NULL, LAZYCC_LOGIC, width);
#else
  and_internal(s, width);
#endif
}

def_REHelper(xor) {
  rtl_xor(s, ddest, ddest, dsrc1);
#ifdef LAZY_CC
  rtl_set_lazycc(s, ddest, NULL, NULL, LAZYCC_LOGIC, width);
#else
  rtl_update_ZFSF(s, ddest, width);
  rtl_mv(s, &cpu.CF, rz);
  rtl_mv(s, &cpu.OF, rz);
#endif
}

def_REHelper(setcc) {
  uint32_t cc = s->isa.opcode & 0xf;
#ifdef LAZY_CC
  rtl_lazy_setcc(s, ddest, cc);
#else
  rtl_setcc(s, ddest, cc);
#endif
}
