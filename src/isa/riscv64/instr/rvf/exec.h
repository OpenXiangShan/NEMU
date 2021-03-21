def_EHelper(flw) {
  rtl_lm(s, ddest, dsrc1, id_src2->imm, 4, MMU_DYNAMIC);
  rtl_fsr(s, ddest, ddest, FPCALL_W64);
}

def_EHelper(fsw) {
  rtl_fbox(s, s0, ddest);
  rtl_sm(s, s0, dsrc1, id_src2->imm, 4, MMU_DYNAMIC);
}

#define def_fop_template(name, op, w) \
  def_EHelper(name) { \
    rtl_mv(s, s0, dsrc1); \
    rtl_hostcall(s, HOSTCALL_FP, s0, dsrc2, FPCALL_CMD(op, w)); \
    rtl_fsr(s, ddest, s0, w); \
  }

def_fop_template(fadds, FPCALL_ADD, FPCALL_W32)
def_fop_template(fsubs, FPCALL_SUB, FPCALL_W32)
def_fop_template(fmuls, FPCALL_MUL, FPCALL_W32)
def_fop_template(fdivs, FPCALL_DIV, FPCALL_W32)

def_EHelper(fsqrts) {
  rtl_hostcall(s, HOSTCALL_FP, ddest, dsrc1, FPCALL_CMD(FPCALL_SQRT, FPCALL_W32));
  rtl_fsr(s, ddest, ddest, FPCALL_W32);
}

def_EHelper(fles) {
  rtl_mv(s, ddest, dsrc1);
  rtl_hostcall(s, HOSTCALL_FP, ddest, dsrc2, FPCALL_CMD(FPCALL_LE, FPCALL_W32));
}

def_EHelper(flts) {
  rtl_mv(s, ddest, dsrc1);
  rtl_hostcall(s, HOSTCALL_FP, ddest, dsrc2, FPCALL_CMD(FPCALL_LT, FPCALL_W32));
}

def_EHelper(feqs) {
  rtl_mv(s, ddest, dsrc1);
  rtl_hostcall(s, HOSTCALL_FP, ddest, dsrc2, FPCALL_CMD(FPCALL_EQ, FPCALL_W32));
}

def_EHelper(fcvt_s_w) {
  rtl_hostcall(s, HOSTCALL_FP, ddest, dsrc1, FPCALL_CMD(FPCALL_I32ToF, FPCALL_W32));
  rtl_fsr(s, ddest, ddest, FPCALL_W32);
}

def_EHelper(fcvt_s_wu) {
  rtl_hostcall(s, HOSTCALL_FP, ddest, dsrc1, FPCALL_CMD(FPCALL_U32ToF, FPCALL_W32));
  rtl_fsr(s, ddest, ddest, FPCALL_W32);
}

def_EHelper(fcvt_s_l) {
  rtl_hostcall(s, HOSTCALL_FP, ddest, dsrc1, FPCALL_CMD(FPCALL_I64ToF, FPCALL_W32));
  rtl_fsr(s, ddest, ddest, FPCALL_W32);
}

def_EHelper(fcvt_s_lu) {
  rtl_hostcall(s, HOSTCALL_FP, ddest, dsrc1, FPCALL_CMD(FPCALL_U64ToF, FPCALL_W32));
  rtl_fsr(s, ddest, ddest, FPCALL_W32);
}

def_EHelper(fcvt_w_s) {
  rtl_hostcall(s, HOSTCALL_FP, ddest, dsrc1, FPCALL_CMD(FPCALL_FToI32, FPCALL_W32));
  rtl_sext(s, ddest, ddest, 4);
}

def_EHelper(fcvt_wu_s) {
  rtl_hostcall(s, HOSTCALL_FP, ddest, dsrc1, FPCALL_CMD(FPCALL_FToU32, FPCALL_W32));
  rtl_sext(s, ddest, ddest, 4);
}

def_EHelper(fcvt_l_s) {
  rtl_hostcall(s, HOSTCALL_FP, ddest, dsrc1, FPCALL_CMD(FPCALL_FToI64, FPCALL_W32));
}

def_EHelper(fcvt_lu_s) {
  rtl_hostcall(s, HOSTCALL_FP, ddest, dsrc1, FPCALL_CMD(FPCALL_FToU64, FPCALL_W32));
}

def_EHelper(fmv_x_w) {
  rtl_sext(s, ddest, dsrc1, 4);
}

def_EHelper(fmv_w_x) {
  rtl_fsr(s, ddest, dsrc1, FPCALL_W32);
}
