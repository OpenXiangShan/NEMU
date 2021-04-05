#define def_RWBHelper(name) static inline void concat(rt_wb_, name) (Decode *s, int width)

def_RWBHelper(r) {
  rtl_sr(s, id_dest->reg, ddest, width);
}
