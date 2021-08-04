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

def_fop_template(faddd, FPCALL_ADD, FPCALL_W64)
def_fop_template(fsubd, FPCALL_SUB, FPCALL_W64)
def_fop_template(fmuld, FPCALL_MUL, FPCALL_W64)
def_fop_template(fdivd, FPCALL_DIV, FPCALL_W64)
def_fop_template(fmaxd, FPCALL_MAX, FPCALL_W64)
def_fop_template(fmind, FPCALL_MIN, FPCALL_W64)

def_EHelper(fsqrtd) {
  rtl_hostcall(s, HOSTCALL_FP, ddest, dsrc1, rz, FPCALL_CMD(FPCALL_SQRT, FPCALL_W64));
  rtl_fsr(s, ddest, ddest, FPCALL_W64);
}

def_EHelper(fmaddd) {
  rtl_mv(s, s0, &fpreg_l(s->isa.instr.fp.funct5)); // rs3
  rtl_hostcall(s, HOSTCALL_FP, s0, dsrc1, dsrc2, FPCALL_CMD(FPCALL_MADD, FPCALL_W64));
  rtl_fsr(s, ddest, s0, FPCALL_W64);
}

def_EHelper(fmsubd) {
  rtl_mv(s, s0, &fpreg_l(s->isa.instr.fp.funct5)); // rs3
  rtl_xori(s, s0, s0, F64_SIGN);
  rtl_hostcall(s, HOSTCALL_FP, s0, dsrc1, dsrc2, FPCALL_CMD(FPCALL_MADD, FPCALL_W64));
  rtl_fsr(s, ddest, s0, FPCALL_W64);
}

def_EHelper(fnmsubd) {
  rtl_mv(s, s0, &fpreg_l(s->isa.instr.fp.funct5)); // rs3
  rtl_xori(s, s1, dsrc1, F64_SIGN);
  rtl_hostcall(s, HOSTCALL_FP, s0, s1, dsrc2, FPCALL_CMD(FPCALL_MADD, FPCALL_W64));
  rtl_fsr(s, ddest, s0, FPCALL_W64);
}

def_EHelper(fnmaddd) {
  rtl_mv(s, s0, &fpreg_l(s->isa.instr.fp.funct5)); // rs3
  rtl_xori(s, s1, dsrc1, F64_SIGN);
  rtl_xori(s, s0, s0, F64_SIGN);
  rtl_hostcall(s, HOSTCALL_FP, s0, s1, dsrc2, FPCALL_CMD(FPCALL_MADD, FPCALL_W64));
  rtl_fsr(s, ddest, s0, FPCALL_W64);
}

def_EHelper(fled) {
  rtl_hostcall(s, HOSTCALL_FP, ddest, dsrc1, dsrc2, FPCALL_CMD(FPCALL_LE, FPCALL_W64));
}

def_EHelper(fltd) {
  rtl_hostcall(s, HOSTCALL_FP, ddest, dsrc1, dsrc2, FPCALL_CMD(FPCALL_LT, FPCALL_W64));
}

def_EHelper(feqd) {
  rtl_hostcall(s, HOSTCALL_FP, ddest, dsrc1, dsrc2, FPCALL_CMD(FPCALL_EQ, FPCALL_W64));
}

def_EHelper(fcvt_d_w) {
  rtl_hostcall(s, HOSTCALL_FP, ddest, dsrc1, rz, FPCALL_CMD(FPCALL_I32ToF, FPCALL_W64));
  rtl_fsr(s, ddest, ddest, FPCALL_W64);
}

def_EHelper(fcvt_d_wu) {
  rtl_hostcall(s, HOSTCALL_FP, ddest, dsrc1, rz, FPCALL_CMD(FPCALL_U32ToF, FPCALL_W64));
  rtl_fsr(s, ddest, ddest, FPCALL_W64);
}

def_EHelper(fcvt_d_l) {
  rtl_hostcall(s, HOSTCALL_FP, ddest, dsrc1, rz, FPCALL_CMD(FPCALL_I64ToF, FPCALL_W64));
  rtl_fsr(s, ddest, ddest, FPCALL_W64);
}

def_EHelper(fcvt_d_lu) {
  rtl_hostcall(s, HOSTCALL_FP, ddest, dsrc1, rz, FPCALL_CMD(FPCALL_U64ToF, FPCALL_W64));
  rtl_fsr(s, ddest, ddest, FPCALL_W64);
}

def_EHelper(fcvt_w_d) {
  rtl_hostcall(s, HOSTCALL_FP, ddest, dsrc1, rz, FPCALL_CMD(FPCALL_FToI32, FPCALL_W64));
  rtl_sext(s, ddest, ddest, 4);
}

def_EHelper(fcvt_wu_d) {
  rtl_hostcall(s, HOSTCALL_FP, ddest, dsrc1, rz, FPCALL_CMD(FPCALL_FToU32, FPCALL_W64));
  rtl_sext(s, ddest, ddest, 4);
}

def_EHelper(fcvt_l_d) {
  rtl_hostcall(s, HOSTCALL_FP, ddest, dsrc1, rz, FPCALL_CMD(FPCALL_FToI64, FPCALL_W64));
}

def_EHelper(fcvt_lu_d) {
  rtl_hostcall(s, HOSTCALL_FP, ddest, dsrc1, rz, FPCALL_CMD(FPCALL_FToU64, FPCALL_W64));
}

def_EHelper(fcvt_d_s) {
  rtl_hostcall(s, HOSTCALL_FP, ddest, dsrc1, rz, FPCALL_CMD(FPCALL_F32ToF64, FPCALL_W64));
  rtl_fsr(s, ddest, ddest, FPCALL_W64);
}

def_EHelper(fcvt_s_d) {
  rtl_hostcall(s, HOSTCALL_FP, ddest, dsrc1, rz, FPCALL_CMD(FPCALL_F64ToF32, FPCALL_W64));
  rtl_fsr(s, ddest, ddest, FPCALL_W32);
}

def_EHelper(fsgnjd) {
  rtl_andi(s, s0, dsrc1, ~F64_SIGN);
  rtl_andi(s, ddest, dsrc2, F64_SIGN);
  rtl_or(s, ddest, s0, ddest);
  rtl_fsr(s, ddest, ddest, FPCALL_W64);
}

def_EHelper(fsgnjnd) {
  rtl_andi(s, s0, dsrc1, ~F64_SIGN);
  rtl_xori(s, ddest, dsrc2, F64_SIGN);
  rtl_andi(s, ddest, ddest, F64_SIGN);
  rtl_or(s, ddest, s0, ddest);
  rtl_fsr(s, ddest, ddest, FPCALL_W64);
}

def_EHelper(fsgnjxd) {
  rtl_andi(s, s0, dsrc1, ~F64_SIGN);
  rtl_xor(s, ddest, dsrc1, dsrc2);
  rtl_andi(s, ddest, ddest, F64_SIGN);
  rtl_or(s, ddest, s0, ddest);
  rtl_fsr(s, ddest, ddest, FPCALL_W64);
}

def_EHelper(fmv_x_d) {
  rtl_sext(s, ddest, dsrc1, 8);
}

def_EHelper(fmv_d_x) {
  rtl_fsr(s, ddest, dsrc1, FPCALL_W64);
}

def_EHelper(fclassd) {
  rtl_fclass(s, ddest, dsrc1, FPCALL_W64);
}
