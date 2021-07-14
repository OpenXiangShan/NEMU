def_EHelper(fld1) {
  rtl_fli(s, dfdest, 0x3ff0000000000000ull);
  ftop_push();
}

def_EHelper(fldz) {
  rtl_fli(s, dfdest, 0);
  ftop_push();
}

def_EHelper(fldl2e) {
  rtl_fli(s, dfdest, 0x3FF71547652B82FEull);
  ftop_push();
}

def_EHelper(fldln2) {
  rtl_fli(s, dfdest, 0x3FE62E42FEFA39EFull);
  ftop_push();
}

def_EHelper(flds) {
  rt_decode_mem(s, id_dest, false, 0);
  rtl_flm(s, dfdest, &s->isa.mbr, s->isa.moff, 4, MMU_DYNAMIC);
  rtl_fcvt_f32_to_f64(s, dfdest, dfdest);
  ftop_push();
}

def_EHelper(fldl) {
  rt_decode_mem(s, id_dest, false, 0);
  rtl_flm(s, dfdest, &s->isa.mbr, s->isa.moff, 8, MMU_DYNAMIC);
  ftop_push();
}

def_EHelper(fld) {
  rtl_fmv(s, dfdest, dfsrc1);
  ftop_push();
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

def_EHelper(fstps) {
  rt_decode_mem(s, id_dest, false, 0);
  rtl_fcvt_f64_to_f32(s, &s->isa.fptmp, dfsrc1);
  rtl_fsm(s, &s->isa.fptmp, &s->isa.mbr, s->isa.moff, 4, MMU_DYNAMIC);
  ftop_pop();
}

def_EHelper(fstpl) {
  rt_decode_mem(s, id_dest, false, 0);
  rtl_fsm(s, dfsrc1, &s->isa.mbr, s->isa.moff, 8, MMU_DYNAMIC);
  ftop_pop();
}

def_EHelper(fstp) {
  rtl_fmv(s, dfdest, dfsrc1);
  ftop_pop();
}

def_EHelper(fxch) {
  rtl_fmv(s, &s->isa.fptmp, dfsrc1);
  rtl_fmv(s, dfsrc1, dfdest);
  rtl_fmv(s, dfdest, &s->isa.fptmp);
}

def_EHelper(fildl) {
  rt_decode_mem(s, id_dest, false, 0);
  rtl_lm(s, s0, &s->isa.mbr, s->isa.moff, 4, MMU_DYNAMIC);
  rtl_fcvt_i32_to_f64(s, dfdest, s0);
  ftop_push();
}

def_EHelper(fildll) {
  rt_decode_mem(s, id_dest, false, 0);
  rtl_flm(s, &s->isa.tmp64, &s->isa.mbr, s->isa.moff, 8, MMU_DYNAMIC);
  rtl_fcvt_i64_to_f64(s, dfdest, &s->isa.tmp64);
  ftop_push();
}

def_EHelper(fistl) {
  rt_decode_mem(s, id_dest, false, 0);
  rtl_fcvt_f64_to_i32(s, s0, dfsrc1);
  rtl_sm(s, s0, &s->isa.mbr, s->isa.moff, 4, MMU_DYNAMIC);
}

def_EHelper(fistpl) {
  rt_decode_mem(s, id_dest, false, 0);
  rtl_fcvt_f64_to_i32(s, s0, dfsrc1);
  rtl_sm(s, s0, &s->isa.mbr, s->isa.moff, 4, MMU_DYNAMIC);
  ftop_pop();
}

def_EHelper(fistpll) {
  rt_decode_mem(s, id_dest, false, 0);
  rtl_fcvt_f64_to_i64(s, &s->isa.tmp64, dfsrc1);
  rtl_fsm(s, &s->isa.tmp64, &s->isa.mbr, s->isa.moff, 8, MMU_DYNAMIC);
  ftop_pop();
}
