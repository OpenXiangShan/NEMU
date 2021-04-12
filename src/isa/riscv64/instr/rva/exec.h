#define def_AMO_EHelper(name) \
def_EHelper(name) { \
  extern void rtl_amo_slow_path(Decode *s, rtlreg_t *dest, const rtlreg_t *src1, const rtlreg_t *src2); \
  rtl_amo_slow_path(s, ddest, dsrc1, dsrc2); \
}

#ifdef CONFIG_DEBUG
#define AMO_LIST(f, s) \
  f(concat3(lr     , _, s)) \
  f(concat3(sc     , _, s)) \
  f(concat3(amoswap, _, s)) \
  f(concat3(amoadd , _, s)) \
  f(concat3(amoor  , _, s)) \
  f(concat3(amoand , _, s)) \
  f(concat3(amoxor , _, s)) \
  f(concat3(amomaxu, _, s)) \
  f(concat3(amomax , _, s)) \
  f(concat3(amominu, _, s)) \
  f(concat3(amomin , _, s))

AMO_LIST(def_AMO_EHelper, d)
AMO_LIST(def_AMO_EHelper, w)
#else
def_AMO_EHelper(atomic)
#endif
