def_THelper(fop_d) {
  int funct4 = s->isa.instr.fp.funct5 & 0b01111;
  switch (funct4) {
    TAB(0b0000, faddd)   TAB(0b0001, fsubd)
    TAB(0b0010, fmuld)   TAB(0b0011, fdivd)
//    TAB(0b0100, fsgnjs)  TAB(0b0101, fmin_fmax)
//    TAB(0b1000, fcvt_F_to_F)   TAB(0b1011, fsqrt)
  }
  return EXEC_ID_inv;
}

def_THelper(fop_gpr_d) {
  int funct4 = s->isa.instr.fp.funct5 & 0b01111;
  switch (funct4) {
//    IDTAB(0b0100, F_fpr_to_gpr, fcmp)
//    IDTAB(0b1000, F_fpr_to_gpr, fcvt_F_to_G) IDTAB(0b1010, F_gpr_to_fpr, fcvt_G_to_F)
//    IDTAB(0b1100, F_fpr_to_gpr, fmv_F_to_G)  IDTAB(0b1110, F_gpr_to_fpr, fmv_G_to_F)
  }
  return EXEC_ID_inv;
}
