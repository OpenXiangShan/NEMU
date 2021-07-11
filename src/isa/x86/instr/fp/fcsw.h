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
  rt_decode_mem(s, id_dest, false, 0);
  rtl_lm(s, s0, &s->isa.mbr, s->isa.moff, 2, MMU_DYNAMIC);

  int rm_x86 = BITS(*s0, 11, 10);
  int rm_fpcall = 0;
  switch (rm_x86) {
    case 0b00: rm_fpcall = FPCALL_RM_RNE; break;
    case 0b01: rm_fpcall = FPCALL_RM_RDN; break;
    case 0b10: rm_fpcall = FPCALL_RM_RUP; break;
    case 0b11: rm_fpcall = FPCALL_RM_RTZ; break;
  }

  void fp_set_rm(uint32_t rm);
  fp_set_rm(rm_fpcall);
}
