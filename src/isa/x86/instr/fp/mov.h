def_EHelper(fld_const) {
  rtl_fpcall(s, FPCALL_LOADCONST, dfdest, NULL, NULL, id_src1->val);
  ftop_update(s);
}

def_EHelper(flds) {
  rt_decode_mem(s, id_dest, false, 0);
  rtl_flm(s, dfdest, &s->isa.mbr, s->isa.moff, 4, MMU_DYNAMIC);
  rtl_fcvt_f32_to_f64(s, dfdest, dfdest);
  ftop_update(s);
}

def_EHelper(fldl) {
  rt_decode_mem(s, id_dest, false, 0);
  rtl_flm(s, dfdest, &s->isa.mbr, s->isa.moff, 8, MMU_DYNAMIC);
  ftop_update(s);
}

def_EHelper(fld) {
  rtl_fmv(s, dfdest, dfsrc1);
  ftop_update(s);
}

def_EHelper(fstl) {
  rt_decode_mem(s, id_dest, false, 0);
  rtl_fsm(s, dfsrc1, &s->isa.mbr, s->isa.moff, 8, MMU_DYNAMIC);
}

def_EHelper(fsts) {
  rt_decode_mem(s, id_dest, false, 0);
  rtl_fcvt_f64_to_f32(s, &s->isa.fptmp, dfsrc1);
  rtl_fsm(s, &s->isa.fptmp, &s->isa.mbr, s->isa.moff, 4, MMU_DYNAMIC);
}

def_EHelper(fst) {
  rtl_fmv(s, dfdest, dfsrc1);
}

def_EHelper(fstps) {
  rt_decode_mem(s, id_dest, false, 0);
  rtl_fcvt_f64_to_f32(s, &s->isa.fptmp, dfsrc1);
  rtl_fsm(s, &s->isa.fptmp, &s->isa.mbr, s->isa.moff, 4, MMU_DYNAMIC);
  ftop_update(s);
}

def_EHelper(fstpl) {
  rt_decode_mem(s, id_dest, false, 0);
  rtl_fsm(s, dfsrc1, &s->isa.mbr, s->isa.moff, 8, MMU_DYNAMIC);
  ftop_update(s);
}

def_EHelper(fstp) {
  rtl_fmv(s, dfdest, dfsrc1);
  ftop_update(s);
}

def_EHelper(fxch) {
  rtl_fmv(s, &s->isa.fptmp, dfsrc1);
  rtl_fmv(s, dfsrc1, dfdest);
  rtl_fmv(s, dfdest, &s->isa.fptmp);
}

def_EHelper(filds) {
  rt_decode_mem(s, id_dest, false, 0);
  rtl_lms(s, s0, &s->isa.mbr, s->isa.moff, 2, MMU_DYNAMIC);
  rtl_fcvt_i32_to_f64(s, dfdest, s0);
  ftop_update(s);
}

def_EHelper(fildl) {
  rt_decode_mem(s, id_dest, false, 0);
  rtl_lm(s, s0, &s->isa.mbr, s->isa.moff, 4, MMU_DYNAMIC);
  rtl_fcvt_i32_to_f64(s, dfdest, s0);
  ftop_update(s);
}

def_EHelper(fildll) {
  rt_decode_mem(s, id_dest, false, 0);
  rtl_fpcall(s, FPCALL_FILDLL, dfdest, NULL, &s->isa.mbr, s->isa.moff);
  ftop_update(s);
}

def_EHelper(fistl) {
  rt_decode_mem(s, id_dest, false, 0);
  rtl_fcvt_f64_to_i32(s, s0, dfsrc1);
  rtl_sm(s, s0, &s->isa.mbr, s->isa.moff, 4, MMU_DYNAMIC);
}

def_EHelper(fistps) {
  rt_decode_mem(s, id_dest, false, 0);
  rtl_fcvt_f64_to_i32(s, s0, dfsrc1);
  rtl_sm(s, s0, &s->isa.mbr, s->isa.moff, 2, MMU_DYNAMIC);
  ftop_update(s);
}

def_EHelper(fistpl) {
  rt_decode_mem(s, id_dest, false, 0);
  rtl_fcvt_f64_to_i32(s, s0, dfsrc1);
  rtl_sm(s, s0, &s->isa.mbr, s->isa.moff, 4, MMU_DYNAMIC);
  ftop_update(s);
}

def_EHelper(fistpll) {
  rt_decode_mem(s, id_dest, false, 0);
  rtl_fpcall(s, FPCALL_FISTLL, dfsrc1, NULL, &s->isa.mbr, s->isa.moff);
  ftop_update(s);
}

def_rtl(fcmovcc, fpreg_t *dest, const fpreg_t *src1);

def_EHelper(fcmovb)   { rtl_fcmovcc(s, dfdest, dfsrc1); }
def_EHelper(fcmove)   { rtl_fcmovcc(s, dfdest, dfsrc1); }
def_EHelper(fcmovbe)  { rtl_fcmovcc(s, dfdest, dfsrc1); }
def_EHelper(fcmovu)   { rtl_fcmovcc(s, dfdest, dfsrc1); }
def_EHelper(fcmovnb)  { rtl_fcmovcc(s, dfdest, dfsrc1); }
def_EHelper(fcmovne)  { rtl_fcmovcc(s, dfdest, dfsrc1); }
def_EHelper(fcmovnbe) { rtl_fcmovcc(s, dfdest, dfsrc1); }
def_EHelper(fcmovnu)  { rtl_fcmovcc(s, dfdest, dfsrc1); }
