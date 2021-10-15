def_rtl(fcmp, const fpreg_t *src1, const fpreg_t *src2);
def_rtl(fcmp_fsw, rtlreg_t *dest, const fpreg_t *src1, const fpreg_t *src2);

def_EHelper(fucomi) {
  rtl_fcmp(s, dfsrc1, dfdest);
}

def_EHelper(fucomip) {
  rtl_fcmp(s, dfsrc1, dfdest);
  ftop_pop();
}

def_EHelper(fcomi) {
  rtl_fcmp(s, dfsrc1, dfdest);
}

def_EHelper(fcomip) {
  rtl_fcmp(s, dfsrc1, dfdest);
  ftop_pop();
}

def_EHelper(fucomp) {
  rtl_fcmp_fsw(s, &cpu.fsw, dfsrc1, dfdest);
  ftop_pop();
}

def_EHelper(fucompp) {
  rtl_fcmp_fsw(s, &cpu.fsw, dfdest, dfsrc1);
  ftop_pop2();
}

def_EHelper(fcoml) {
  rt_decode_mem(s, id_dest, false, 0);
  rtl_flm(s, &s->isa.fptmp, &s->isa.mbr, s->isa.moff, 8, MMU_DYNAMIC);
  rtl_fcmp_fsw(s, &cpu.fsw, dfdest, &s->isa.fptmp);
}

def_EHelper(fcompl) {
  rt_decode_mem(s, id_dest, false, 0);
  rtl_flm(s, &s->isa.fptmp, &s->isa.mbr, s->isa.moff, 8, MMU_DYNAMIC);
  rtl_fcmp_fsw(s, &cpu.fsw, dfdest, &s->isa.fptmp);
  ftop_pop();
}
