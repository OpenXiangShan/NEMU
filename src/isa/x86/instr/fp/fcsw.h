def_EHelper(fxam) {
  // TODO
}

def_EHelper(fnstsw) {
  // TODO
  rtl_li(s, s0, 0);
  rtl_sr(s, R_AX, s0, 2);
}

def_EHelper(fnstcw) {
  // TODO
  rt_decode_mem(s, id_dest, false, 0);
  rtl_li(s, s0, 0);
  rtl_sm(s, s0, &s->isa.mbr, s->isa.moff, 2, MMU_DYNAMIC);
}

def_EHelper(fldcw) {
  // TODO
}
