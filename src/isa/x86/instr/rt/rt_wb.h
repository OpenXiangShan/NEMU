#define def_RWBHelper(name) static inline void concat(rt_wb_, name) (Decode *s, int width)

def_RWBHelper(r) {
  rtl_sr(s, id_dest->reg, ddest, width);
}

def_RWBHelper(E) {
  if (!s->isa.is_rm_memory) { rtl_sr(s, id_dest->reg, ddest, width); }
  else { rtl_sm(s, ddest, s->isa.mbase, s->isa.moff, width, MMU_DYNAMIC); }
}
