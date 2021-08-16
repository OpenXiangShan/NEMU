#define F64_SIGN ((uint64_t)1 << 63)

def_EHelper(fld) {
  rtl_lm(s, ddest, dsrc1, id_src2->imm, 8, MMU_DIRECT);
  rtl_fsr(s, ddest, ddest, FPCALL_W64);
}

def_EHelper(fsd) {
  rtl_sm(s, ddest, dsrc1, id_src2->imm, 8, MMU_DIRECT);
}

def_EHelper(fld_mmu) {
  rtl_lm(s, ddest, dsrc1, id_src2->imm, 8, MMU_TRANSLATE);
  rtl_fsr(s, ddest, ddest, FPCALL_W64);
}

def_EHelper(fsd_mmu) {
  rtl_sm(s, ddest, dsrc1, id_src2->imm, 8, MMU_TRANSLATE);
}

def_fop_template(faddd, FPCALL_W64)
def_fop_template(fsubd, FPCALL_W64)
def_fop_template(fmuld, FPCALL_W64)
def_fop_template(fdivd, FPCALL_W64)
def_fop_template(fmaxd, FPCALL_W64)
def_fop_template(fmind, FPCALL_W64)

def_EHelper(fsqrtd) {
  rtl_fsqrtd(s, ddest, dsrc1);
  rtl_fsr(s, ddest, ddest, FPCALL_W64);
}

def_EHelper(fmaddd) {
  rtl_mv(s, s0, &fpr(s->isa.instr.fp.funct5)); // rs3
  rtl_fmaddd(s, s0, dsrc1, dsrc2);
  rtl_fsr(s, ddest, s0, FPCALL_W64);
}

def_EHelper(fmsubd) {
  rtl_mv(s, s0, &fpr(s->isa.instr.fp.funct5)); // rs3
  rtl_xori(s, s0, s0, F64_SIGN);
  rtl_fmaddd(s, s0, dsrc1, dsrc2);
  rtl_fsr(s, ddest, s0, FPCALL_W64);
}

def_EHelper(fnmsubd) {
  rtl_mv(s, s0, &fpr(s->isa.instr.fp.funct5)); // rs3
  rtl_xori(s, s1, dsrc1, F64_SIGN);
  rtl_fmaddd(s, s0, s1, dsrc2);
  rtl_fsr(s, ddest, s0, FPCALL_W64);
}

def_EHelper(fnmaddd) {
  rtl_mv(s, s0, &fpr(s->isa.instr.fp.funct5)); // rs3
  rtl_fmaddd(s, s0, dsrc1, dsrc2);
  rtl_xori(s, s0, s0, F64_SIGN);
  rtl_fsr(s, ddest, s0, FPCALL_W64);
}

def_EHelper(fled) {
  rtl_fled(s, ddest, dsrc1, dsrc2);
}

def_EHelper(fltd) {
  rtl_fltd(s, ddest, dsrc1, dsrc2);
}

def_EHelper(feqd) {
  rtl_feqd(s, ddest, dsrc1, dsrc2);
}

def_EHelper(fcvt_d_w) {
  rtl_fcvt_i32_to_f64(s, ddest, dsrc1);
  rtl_fsr(s, ddest, ddest, FPCALL_W64);
}

def_EHelper(fcvt_d_wu) {
  rtl_fcvt_u32_to_f64(s, ddest, dsrc1);
  rtl_fsr(s, ddest, ddest, FPCALL_W64);
}

def_EHelper(fcvt_d_l) {
  rtl_fcvt_i64_to_f64(s, ddest, dsrc1);
  rtl_fsr(s, ddest, ddest, FPCALL_W64);
}

def_EHelper(fcvt_d_lu) {
  rtl_fcvt_u64_to_f64(s, ddest, dsrc1);
  rtl_fsr(s, ddest, ddest, FPCALL_W64);
}

def_EHelper(fcvt_w_d) {
  rtl_fcvt_f64_to_i32(s, ddest, dsrc1);
  rtl_sext(s, ddest, ddest, 4);
}

def_EHelper(fcvt_wu_d) {
  rtl_fcvt_f64_to_u32(s, ddest, dsrc1);
  rtl_sext(s, ddest, ddest, 4);
}

def_EHelper(fcvt_l_d) {
  rtl_fcvt_f64_to_i64(s, ddest, dsrc1);
}

def_EHelper(fcvt_lu_d) {
  rtl_fcvt_f64_to_u64(s, ddest, dsrc1);
}

def_EHelper(fcvt_d_s) {
  rtl_fcvt_f32_to_f64(s, ddest, dsrc1);
  rtl_fsr(s, ddest, ddest, FPCALL_W64);
}

def_EHelper(fcvt_s_d) {
  rtl_fcvt_f64_to_f32(s, ddest, dsrc1);
  rtl_fsr(s, ddest, ddest, FPCALL_W32);
}

def_EHelper(fsgnjd) {
  rtl_andi(s, s0, dsrc1, ~F64_SIGN);
  rtl_andi(s, ddest, dsrc2, F64_SIGN);
  rtl_or(s, ddest, s0, ddest);
}

def_EHelper(fsgnjnd) {
  rtl_andi(s, s0, dsrc1, ~F64_SIGN);
  rtl_xori(s, ddest, dsrc2, F64_SIGN);
  rtl_andi(s, ddest, ddest, F64_SIGN);
  rtl_or(s, ddest, s0, ddest);
}

def_EHelper(fsgnjxd) {
  rtl_andi(s, s0, dsrc1, ~F64_SIGN);
  rtl_xor(s, ddest, dsrc1, dsrc2);
  rtl_andi(s, ddest, ddest, F64_SIGN);
  rtl_or(s, ddest, s0, ddest);
}

def_EHelper(fmv_x_d) {
  rtl_sext(s, ddest, dsrc1, 8);
}

def_EHelper(fmv_d_x) {
  rtl_fsr(s, ddest, dsrc1, FPCALL_W64);
}
