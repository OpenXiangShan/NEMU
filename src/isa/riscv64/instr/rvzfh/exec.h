#define F16_SIGN ((uint64_t)1 << 15)
// CONFIG_RV_ZFH_MIN
#ifdef CONFIG_RV_ZFH_MIN
def_EHelper(flh) {
  rtl_lm(s, ddest, dsrc1, id_src2->imm, 2, MMU_DIRECT);
  rtl_fsr(s, ddest, ddest, FPCALL_W16);
}

def_EHelper(fsh) {
  rtl_sm(s, ddest, dsrc1, id_src2->imm, 2, MMU_DIRECT);
}

def_EHelper(flh_mmu) {
  rtl_lm(s, ddest, dsrc1, id_src2->imm, 2, MMU_TRANSLATE);
  rtl_fsr(s, ddest, ddest, FPCALL_W16);
}

def_EHelper(fsh_mmu) {
  rtl_sm(s, ddest, dsrc1, id_src2->imm, 2, MMU_TRANSLATE);
}

def_EHelper(fmv_x_h) {
  rtl_sext(s, ddest, dsrc1, 2);
}

def_EHelper(fmv_h_x) {
  rtl_fsr(s, ddest, dsrc1, FPCALL_W16);
}

def_EHelper(fcvt_h_s) {
  rtl_hostcall(s, HOSTCALL_FP, ddest, dsrc1, rz, FPCALL_CMD(FPCALL_F32ToF16, FPCALL_W32));
  rtl_fsr(s, ddest, ddest, FPCALL_W16);
}

def_EHelper(fcvt_s_h) {
  rtl_hostcall(s, HOSTCALL_FP, ddest, dsrc1, rz, FPCALL_CMD(FPCALL_F16ToF32, FPCALL_W16));
  rtl_fsr(s, ddest, ddest, FPCALL_W32);
}

def_EHelper(fcvt_h_d) {
  rtl_hostcall(s, HOSTCALL_FP, ddest, dsrc1, rz, FPCALL_CMD(FPCALL_F64ToF16, FPCALL_W64));
  rtl_fsr(s, ddest, ddest, FPCALL_W16);
}

def_EHelper(fcvt_d_h) {
  rtl_hostcall(s, HOSTCALL_FP, ddest, dsrc1, rz, FPCALL_CMD(FPCALL_F16ToF64, FPCALL_W16));
  rtl_fsr(s, ddest, ddest, FPCALL_W64);
}
#endif
#ifdef CONFIG_RV_ZFH
#define def_fop_template(name, op, w) \
  def_EHelper(name) { \
    rtl_hostcall(s, HOSTCALL_FP, ddest, dsrc1, dsrc2, FPCALL_CMD(op, w)); \
    rtl_fsr(s, ddest, ddest, w); \
  }
def_fop_template(faddh, FPCALL_ADD, FPCALL_W16)
def_fop_template(fsubh, FPCALL_SUB, FPCALL_W16)
def_fop_template(fmulh, FPCALL_MUL, FPCALL_W16)
def_fop_template(fdivh, FPCALL_DIV, FPCALL_W16)
def_fop_template(fmaxh, FPCALL_MAX, FPCALL_W16)
def_fop_template(fminh, FPCALL_MIN, FPCALL_W16)

def_EHelper(fsqrth) {
  rtl_hostcall(s, HOSTCALL_FP, ddest, dsrc1, rz, FPCALL_CMD(FPCALL_SQRT, FPCALL_W16));
  rtl_fsr(s, ddest, ddest, FPCALL_W16);
}
def_EHelper(fmaddh) {
  rtl_mv(s, s0, &fpreg_l(s->isa.instr.fp.funct5)); // rs3
  rtl_hostcall(s, HOSTCALL_FP, s0, dsrc1, dsrc2, FPCALL_CMD(FPCALL_MADD, FPCALL_W16));
  rtl_fsr(s, ddest, s0, FPCALL_W16);
}

def_EHelper(fmsubh) {
  rtl_mv(s, s0, &fpreg_l(s->isa.instr.fp.funct5)); // rs3
  rtl_xori(s, s0, s0, F16_SIGN);
  rtl_hostcall(s, HOSTCALL_FP, s0, dsrc1, dsrc2, FPCALL_CMD(FPCALL_MADD, FPCALL_W16));
  rtl_fsr(s, ddest, s0, FPCALL_W16);
}

def_EHelper(fnmsubh) {
  rtl_mv(s, s0, &fpreg_l(s->isa.instr.fp.funct5)); // rs3
  rtl_xori(s, s1, dsrc1, F16_SIGN);
  rtl_hostcall(s, HOSTCALL_FP, s0, s1, dsrc2, FPCALL_CMD(FPCALL_MADD, FPCALL_W16));
  rtl_fsr(s, ddest, s0, FPCALL_W16);
}

def_EHelper(fnmaddh) {
  rtl_mv(s, s0, &fpreg_l(s->isa.instr.fp.funct5)); // rs3
  rtl_xori(s, s1, dsrc1, F16_SIGN);
  rtl_xori(s, s0, s0, F16_SIGN);
  rtl_hostcall(s, HOSTCALL_FP, s0, s1, dsrc2, FPCALL_CMD(FPCALL_MADD, FPCALL_W16));
  rtl_fsr(s, ddest, s0, FPCALL_W16);
}

def_EHelper(fleh) {
  rtl_hostcall(s, HOSTCALL_FP, ddest, dsrc1, dsrc2, FPCALL_CMD(FPCALL_LE, FPCALL_W16));
  void fp_set_dirty();
  fp_set_dirty();
}

def_EHelper(flth) {
  rtl_hostcall(s, HOSTCALL_FP, ddest, dsrc1, dsrc2, FPCALL_CMD(FPCALL_LT, FPCALL_W16));
  void fp_set_dirty();
  fp_set_dirty();
}

def_EHelper(feqh) {
  rtl_hostcall(s, HOSTCALL_FP, ddest, dsrc1, dsrc2, FPCALL_CMD(FPCALL_EQ, FPCALL_W16));
  void fp_set_dirty();
  fp_set_dirty();
}

def_EHelper(fcvt_h_w) {
  rtl_hostcall(s, HOSTCALL_FP, ddest, dsrc1, rz, FPCALL_CMD(FPCALL_I32ToF, FPCALL_W16));
  rtl_fsr(s, ddest, ddest, FPCALL_W16);
}

def_EHelper(fcvt_h_wu) {
  rtl_hostcall(s, HOSTCALL_FP, ddest, dsrc1, rz, FPCALL_CMD(FPCALL_U32ToF, FPCALL_W16));
  rtl_fsr(s, ddest, ddest, FPCALL_W16);
}

def_EHelper(fcvt_h_l) {
  rtl_hostcall(s, HOSTCALL_FP, ddest, dsrc1, rz, FPCALL_CMD(FPCALL_I64ToF, FPCALL_W16));
  rtl_fsr(s, ddest, ddest, FPCALL_W16);
}

def_EHelper(fcvt_h_lu) {
  rtl_hostcall(s, HOSTCALL_FP, ddest, dsrc1, rz, FPCALL_CMD(FPCALL_U64ToF, FPCALL_W16));
  rtl_fsr(s, ddest, ddest, FPCALL_W16);
}

def_EHelper(fcvt_w_h) {
  rtl_hostcall(s, HOSTCALL_FP, ddest, dsrc1, rz, FPCALL_CMD(FPCALL_FToI32, FPCALL_W16));
  rtl_sext(s, ddest, ddest, 4);
  void fp_set_dirty();
  fp_set_dirty();
}

def_EHelper(fcvt_wu_h) {
  rtl_hostcall(s, HOSTCALL_FP, ddest, dsrc1, rz, FPCALL_CMD(FPCALL_FToU32, FPCALL_W16));
  rtl_sext(s, ddest, ddest, 4);
  void fp_set_dirty();
  fp_set_dirty();
}

def_EHelper(fcvt_l_h) {
  rtl_hostcall(s, HOSTCALL_FP, ddest, dsrc1, rz, FPCALL_CMD(FPCALL_FToI64, FPCALL_W16));
  rtl_fsr(s, ddest, ddest, FPCALL_W64);
  void fp_set_dirty();
  fp_set_dirty();
}

def_EHelper(fcvt_lu_h) {
  rtl_hostcall(s, HOSTCALL_FP, ddest, dsrc1, rz, FPCALL_CMD(FPCALL_FToU64, FPCALL_W16));
  rtl_fsr(s, ddest, ddest, FPCALL_W64);
  void fp_set_dirty();
  fp_set_dirty();
}


def_EHelper(fsgnjh) {
  rtl_hostcall(s, HOSTCALL_FP, ddest, dsrc1, dsrc2, FPCALL_CMD(FPCALL_SGNJ, FPCALL_W16));
  rtl_fsr(s, ddest, ddest, FPCALL_W16);
}

def_EHelper(fsgnjnh) {
  rtl_hostcall(s, HOSTCALL_FP, ddest, dsrc1, dsrc2, FPCALL_CMD(FPCALL_SGNJN, FPCALL_W16));
  rtl_fsr(s, ddest, ddest, FPCALL_W16);
}

def_EHelper(fsgnjxh) {
  rtl_hostcall(s, HOSTCALL_FP, ddest, dsrc1, dsrc2, FPCALL_CMD(FPCALL_SGNJX, FPCALL_W16));
  rtl_fsr(s, ddest, ddest, FPCALL_W16);
}


def_EHelper(fclassh) {
  rtl_fclass(s, ddest, dsrc1, FPCALL_W16);
}
#endif