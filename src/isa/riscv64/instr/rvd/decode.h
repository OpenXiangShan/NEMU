def_THelper(fsgnj_d_dispatch) {
  switch (s->isa.instr.fp.rm) {
    TAB(0b000, fsgnjd)  TAB(0b010, fsgnjxd)
  }
  return EXEC_ID_inv;
}

def_THelper(fop_d) {
  int funct4 = s->isa.instr.fp.funct5 & 0b01111;
  if (funct4 == 0b1000) {
    switch (s->isa.instr.fp.rs2) {
      TAB(0b00000, fcvt_d_s) TAB(0b00001, fcvt_s_d)
    }
    return EXEC_ID_inv;
  }
  switch (funct4) {
    TAB(0b0000, faddd)   TAB(0b0001, fsubd)
    TAB(0b0010, fmuld)   TAB(0b0011, fdivd)
    TAB(0b0100, fsgnj_d_dispatch)
//    TAB(0b0100, fsgnjs)  TAB(0b0101, fmin_fmax)
    TAB(0b1011, fsqrtd)
  }
  return EXEC_ID_inv;
}

def_THelper(fcmp_d_dispatch) {
  switch (s->isa.instr.fp.rm) {
    TAB(0b000, fled)  TAB(0b001, fltd)  TAB(0b010, feqd)
  }
  return EXEC_ID_inv;
}

def_THelper(fmv_d_dispatch) {
  int funct4 = s->isa.instr.fp.funct5 & 0b01111;
  int rm = s->isa.instr.fp.rm;
#define pair(x, y) (((y) << 3) | (x))
  switch (pair(rm, funct4)) {
    TAB(pair(0b000, 0b1100), fmv_x_d)
    TAB(pair(0b000, 0b1110), fmv_d_x)
//    TAB(pair(0b001, 0b1100), fclass)
  }
#undef pair
  return EXEC_ID_inv;
}

def_THelper(fop_gpr_d) {
  int funct4 = s->isa.instr.fp.funct5 & 0b01111;
  switch (funct4) {
    IDTAB(0b0100, fr2r, fcmp_d_dispatch)
  }
  assert((s->isa.instr.fp.rs2 & 0x1c) == 0);
  int sign = s->isa.instr.fp.rs2 & 0x3;
#define pair(x, y) (((y) << 2) | (x))
  switch (pair(sign, funct4)) {
    IDTAB(pair(0b00, 0b1010), r2fr, fcvt_d_w)
    IDTAB(pair(0b01, 0b1010), r2fr, fcvt_d_wu)
    IDTAB(pair(0b10, 0b1010), r2fr, fcvt_d_l)
    IDTAB(pair(0b11, 0b1010), r2fr, fcvt_d_lu)

    IDTAB(pair(0b00, 0b1000), fr2r, fcvt_w_d)
    IDTAB(pair(0b01, 0b1000), fr2r, fcvt_wu_d)
    IDTAB(pair(0b10, 0b1000), fr2r, fcvt_l_d)
    IDTAB(pair(0b11, 0b1000), fr2r, fcvt_lu_d)

    IDTAB(pair(0b00, 0b1100), fr2r, fmv_d_dispatch)
    IDTAB(pair(0b00, 0b1110), r2fr, fmv_d_dispatch)
//    IDTAB(0b1000, fr2r, fcvt_F_to_G) IDTAB(0b1010, r2fr, fcvt_G_to_F)
//    IDTAB(0b1100, fr2r, fmv_F_to_G)  IDTAB(0b1110, r2fr, fmv_G_to_F)
  }
#undef pair
  return EXEC_ID_inv;
}
