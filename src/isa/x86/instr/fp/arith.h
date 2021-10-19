def_EHelper(fadd) {
  rtl_faddd(s, dfdest, dfdest, dfsrc1);
}

def_EHelper(faddl) {
  rt_decode_mem(s, id_dest, false, 0);
  rtl_flm(s, &s->isa.fptmp, &s->isa.mbr, s->isa.moff, 8, MMU_DYNAMIC);
  rtl_faddd(s, dfdest, dfdest, &s->isa.fptmp);
}

def_EHelper(fadds) {
  rt_decode_mem(s, id_dest, false, 0);
  rtl_flm(s, &s->isa.fptmp, &s->isa.mbr, s->isa.moff, 4, MMU_DYNAMIC);
  rtl_fcvt_f32_to_f64(s, &s->isa.fptmp, &s->isa.fptmp);
  rtl_faddd(s, dfdest, dfdest, &s->isa.fptmp);
}

def_EHelper(faddp) {
  rtl_faddd(s, dfdest, dfdest, dfsrc1);
  ftop_pop();
}

def_EHelper(fsub) {
  rtl_fsubd(s, dfdest, dfdest, dfsrc1);
}

def_EHelper(fsubl) {
  rt_decode_mem(s, id_dest, false, 0);
  rtl_flm(s, &s->isa.fptmp, &s->isa.mbr, s->isa.moff, 8, MMU_DYNAMIC);
  rtl_fsubd(s, dfdest, dfdest, &s->isa.fptmp);
}

def_EHelper(fsubs) {
  rt_decode_mem(s, id_dest, false, 0);
  rtl_flm(s, &s->isa.fptmp, &s->isa.mbr, s->isa.moff, 4, MMU_DYNAMIC);
  rtl_fcvt_f32_to_f64(s, &s->isa.fptmp, &s->isa.fptmp);
  rtl_fsubd(s, dfdest, dfdest, &s->isa.fptmp);
}

def_EHelper(fsubp) {
  rtl_fsubd(s, dfdest, dfdest, dfsrc1);
  ftop_pop();
}

def_EHelper(fsubr) {
  rtl_fsubd(s, dfdest, dfsrc1, dfdest);
}

def_EHelper(fsubrs) {
  rt_decode_mem(s, id_dest, false, 0);
  rtl_flm(s, &s->isa.fptmp, &s->isa.mbr, s->isa.moff, 4, MMU_DYNAMIC);
  rtl_fcvt_f32_to_f64(s, &s->isa.fptmp, &s->isa.fptmp);
  rtl_fsubd(s, dfdest, &s->isa.fptmp, dfdest);
}

def_EHelper(fsubrl) {
  rt_decode_mem(s, id_dest, false, 0);
  rtl_flm(s, &s->isa.fptmp, &s->isa.mbr, s->isa.moff, 8, MMU_DYNAMIC);
  rtl_fsubd(s, dfdest, &s->isa.fptmp, dfdest);
}

def_EHelper(fsubrp) {
  rtl_fsubd(s, dfdest, dfsrc1, dfdest);
  ftop_pop();
}


def_EHelper(fmul) {
  rtl_fmuld(s, dfdest, dfdest, dfsrc1);
}

def_EHelper(fmull) {
  rt_decode_mem(s, id_dest, false, 0);
  rtl_flm(s, &s->isa.fptmp, &s->isa.mbr, s->isa.moff, 8, MMU_DYNAMIC);
  rtl_fmuld(s, dfdest, dfdest, &s->isa.fptmp);
}

def_EHelper(fmuls) {
  rt_decode_mem(s, id_dest, false, 0);
  rtl_flm(s, &s->isa.fptmp, &s->isa.mbr, s->isa.moff, 4, MMU_DYNAMIC);
  rtl_fcvt_f32_to_f64(s, &s->isa.fptmp, &s->isa.fptmp);
  rtl_fmuld(s, dfdest, dfdest, &s->isa.fptmp);
}

def_EHelper(fmulp) {
  rtl_fmuld(s, dfdest, dfdest, dfsrc1);
  ftop_pop();
}

def_EHelper(fdiv) {
  rtl_fdivd(s, dfdest, dfdest, dfsrc1);
}

def_EHelper(fdivs) {
  rt_decode_mem(s, id_dest, false, 0);
  rtl_flm(s, &s->isa.fptmp, &s->isa.mbr, s->isa.moff, 4, MMU_DYNAMIC);
  rtl_fcvt_f32_to_f64(s, &s->isa.fptmp, &s->isa.fptmp);
  rtl_fdivd(s, dfdest, dfdest, &s->isa.fptmp);
}

def_EHelper(fdivl) {
  rt_decode_mem(s, id_dest, false, 0);
  rtl_flm(s, &s->isa.fptmp, &s->isa.mbr, s->isa.moff, 8, MMU_DYNAMIC);
  rtl_fdivd(s, dfdest, dfdest, &s->isa.fptmp);
}

def_EHelper(fdivp) {
  rtl_fdivd(s, dfdest, dfdest, dfsrc1);
  ftop_pop();
}

def_EHelper(fdivr) {
  rtl_fdivd(s, dfdest, dfsrc1, dfdest);
}

def_EHelper(fdivrs) {
  rt_decode_mem(s, id_dest, false, 0);
  rtl_flm(s, &s->isa.fptmp, &s->isa.mbr, s->isa.moff, 4, MMU_DYNAMIC);
  rtl_fcvt_f32_to_f64(s, &s->isa.fptmp, &s->isa.fptmp);
  rtl_fdivd(s, dfdest, &s->isa.fptmp, dfdest);
}

def_EHelper(fdivrl) {
  rt_decode_mem(s, id_dest, false, 0);
  rtl_flm(s, &s->isa.fptmp, &s->isa.mbr, s->isa.moff, 8, MMU_DYNAMIC);
  rtl_fdivd(s, dfdest, &s->isa.fptmp, dfdest);
}

def_EHelper(fdivrp) {
  rtl_fdivd(s, dfdest, dfsrc1, dfdest);
  ftop_pop();
}

def_EHelper(fchs) {
  rtl_fneg(s, dfdest, dfdest);
}

def_EHelper(fabs) {
  rtl_fabs(s, dfdest, dfdest);
}

def_EHelper(fsqrt) {
  rtl_fsqrtd(s, dfdest, dfdest);
}

def_EHelper(frndint) {
  rtl_fpcall(s, FPCALL_ROUNDINT, dfdest, dfdest, 0);
}

def_EHelper(f2xm1) {
  rtl_fpcall(s, FPCALL_POW2, dfdest, dfdest, 0);
  rtl_fpcall(s, FPCALL_LOADCONST, &s->isa.fptmp, NULL, 0); // load 1.0
  rtl_fsubd(s, dfdest, dfdest, &s->isa.fptmp);
}

def_EHelper(fscale) {
  rtl_fpcall(s, FPCALL_ROUNDINT, &s->isa.fptmp, dfsrc1, 0);
  rtl_fpcall(s, FPCALL_POW2, &s->isa.fptmp, &s->isa.fptmp, 0);
  rtl_fmuld(s, dfdest, dfdest, &s->isa.fptmp);
}

def_EHelper(fyl2x) {
  rtl_fpcall(s, FPCALL_LOG2, &s->isa.fptmp, dfdest, 0);
  rtl_fmuld(s, dfsrc1, dfsrc1, &s->isa.fptmp);
  ftop_pop();
}

def_EHelper(fyl2xp1) {
  rtl_fpcall(s, FPCALL_LOADCONST, &s->isa.fptmp, NULL, 0); // load 1.0
  rtl_faddd(s, &s->isa.fptmp, &s->isa.fptmp, dfdest);
  rtl_fpcall(s, FPCALL_LOG2, &s->isa.fptmp, &s->isa.fptmp, 0);
  rtl_fmuld(s, dfsrc1, dfsrc1, &s->isa.fptmp);
  ftop_pop();
}

def_EHelper(fprem) {
  rtl_fpcall(s, FPCALL_MOD, dfdest, dfsrc1, 0);
  rtl_andi(s, &cpu.fsw, &cpu.fsw, ~0x4700);
}

def_EHelper(fpatan) {
  rtl_fpcall(s, FPCALL_ATAN, dfsrc1, dfdest, 0);
  ftop_pop();
}
