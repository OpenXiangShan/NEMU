def_EHelper(fld) {
  rtl_lm(s, ddest, dsrc1, id_src2->imm, 8, MMU_DYNAMIC);
  rtl_fsr(s, ddest, ddest, FPCALL_W64);
}

def_EHelper(fsd) {
  rtl_sm(s, ddest, dsrc1, id_src2->imm, 8, MMU_DYNAMIC);
}

def_fop_template(faddd, FPCALL_ADD, FPCALL_W64)
def_fop_template(fsubd, FPCALL_SUB, FPCALL_W64)
def_fop_template(fmuld, FPCALL_MUL, FPCALL_W64)
def_fop_template(fdivd, FPCALL_DIV, FPCALL_W64)

def_EHelper(fsqrtd) {
  rtl_hostcall(s, HOSTCALL_FP, ddest, dsrc1, FPCALL_CMD(FPCALL_SQRT, FPCALL_W64));
  rtl_fsr(s, ddest, ddest, FPCALL_W64);
}

def_EHelper(fled) {
  rtl_mv(s, ddest, dsrc1);
  rtl_hostcall(s, HOSTCALL_FP, ddest, dsrc2, FPCALL_CMD(FPCALL_LE, FPCALL_W64));
}

def_EHelper(fltd) {
  rtl_mv(s, ddest, dsrc1);
  rtl_hostcall(s, HOSTCALL_FP, ddest, dsrc2, FPCALL_CMD(FPCALL_LT, FPCALL_W64));
}

def_EHelper(feqd) {
  rtl_mv(s, ddest, dsrc1);
  rtl_hostcall(s, HOSTCALL_FP, ddest, dsrc2, FPCALL_CMD(FPCALL_EQ, FPCALL_W64));
}

def_EHelper(fcvt_d_w) {
  rtl_hostcall(s, HOSTCALL_FP, ddest, dsrc1, FPCALL_CMD(FPCALL_I32ToF, FPCALL_W64));
  rtl_fsr(s, ddest, ddest, FPCALL_W64);
}

def_EHelper(fcvt_d_wu) {
  rtl_hostcall(s, HOSTCALL_FP, ddest, dsrc1, FPCALL_CMD(FPCALL_U32ToF, FPCALL_W64));
  rtl_fsr(s, ddest, ddest, FPCALL_W64);
}

def_EHelper(fcvt_d_l) {
  rtl_hostcall(s, HOSTCALL_FP, ddest, dsrc1, FPCALL_CMD(FPCALL_I64ToF, FPCALL_W64));
  rtl_fsr(s, ddest, ddest, FPCALL_W64);
}

def_EHelper(fcvt_d_lu) {
  rtl_hostcall(s, HOSTCALL_FP, ddest, dsrc1, FPCALL_CMD(FPCALL_U64ToF, FPCALL_W64));
  rtl_fsr(s, ddest, ddest, FPCALL_W64);
}

def_EHelper(fcvt_w_d) {
  rtl_hostcall(s, HOSTCALL_FP, ddest, dsrc1, FPCALL_CMD(FPCALL_FToI32, FPCALL_W64));
  rtl_sext(s, ddest, ddest, 4);
}

def_EHelper(fcvt_wu_d) {
  rtl_hostcall(s, HOSTCALL_FP, ddest, dsrc1, FPCALL_CMD(FPCALL_FToU32, FPCALL_W64));
  rtl_sext(s, ddest, ddest, 4);
}

def_EHelper(fcvt_l_d) {
  rtl_hostcall(s, HOSTCALL_FP, ddest, dsrc1, FPCALL_CMD(FPCALL_FToI64, FPCALL_W64));
}

def_EHelper(fcvt_lu_d) {
  rtl_hostcall(s, HOSTCALL_FP, ddest, dsrc1, FPCALL_CMD(FPCALL_FToU64, FPCALL_W64));
}

def_EHelper(fcvt_d_s) {
  rtl_hostcall(s, HOSTCALL_FP, ddest, dsrc1, FPCALL_CMD(FPCALL_F32ToF64, FPCALL_W64));
  rtl_fsr(s, ddest, ddest, FPCALL_W64);
}

def_EHelper(fcvt_s_d) {
  rtl_hostcall(s, HOSTCALL_FP, ddest, dsrc1, FPCALL_CMD(FPCALL_F64ToF32, FPCALL_W64));
  rtl_fsr(s, ddest, ddest, FPCALL_W32);
}

def_EHelper(fmv_x_d) {
  rtl_sext(s, ddest, dsrc1, 8);
}

def_EHelper(fmv_d_x) {
  rtl_fsr(s, ddest, dsrc1, FPCALL_W64);
}
