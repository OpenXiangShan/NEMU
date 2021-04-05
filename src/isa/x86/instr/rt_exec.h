#define def_REHelper(name) static inline void concat(rt_exec_, name) (Decode *s)

def_REHelper(mov) {
  rtl_mv(s, ddest, dsrc1);
}
