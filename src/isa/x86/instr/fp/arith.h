def_EHelper(fadd) {
  rtl_faddd(s, dfdest, dfdest, dfsrc1);
}

def_EHelper(faddp) {
  rtl_faddd(s, dfdest, dfdest, dfsrc1);
  ftop_pop();
}

def_EHelper(fsubl) {
  rt_decode_mem(s, id_dest, false, 0);
  rtl_flm(s, &s->isa.fptmp, &s->isa.mbr, s->isa.moff, 8, MMU_DYNAMIC);
  rtl_fsubd(s, dfdest, dfdest, &s->isa.fptmp);
}

def_EHelper(fsubr) {
  rtl_fsubd(s, dfdest, dfsrc1, dfdest);
}

def_EHelper(fmuls) {
  rt_decode_mem(s, id_dest, false, 0);
  rtl_flm(s, &s->isa.fptmp, &s->isa.mbr, s->isa.moff, 4, MMU_DYNAMIC);
  rtl_fcvt_f32_to_f64(s, &s->isa.fptmp, &s->isa.fptmp);
  rtl_fmuld(s, dfdest, dfdest, &s->isa.fptmp);
}

def_EHelper(fdiv) {
  rtl_fdivd(s, dfdest, dfdest, dfsrc1);
}

def_EHelper(fdivp) {
  rtl_fdivd(s, dfdest, dfdest, dfsrc1);
  ftop_pop();
}

def_EHelper(fabs) {
  rtl_fabs(s, dfdest, dfdest);
}
