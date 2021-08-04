#define F32_SIGN ((uint64_t)1 << 31)

def_rtl(fclass, rtlreg_t *, rtlreg_t *, int);

def_EHelper(flw) {
  rtl_lm(s, ddest, dsrc1, id_src2->imm, 4, MMU_DIRECT);
  rtl_fsr(s, ddest, ddest, FPCALL_W32);
}

def_EHelper(fsw) {
  rtl_sm(s, ddest, dsrc1, id_src2->imm, 4, MMU_DIRECT);
}

def_EHelper(flw_mmu) {
  rtl_lm(s, ddest, dsrc1, id_src2->imm, 4, MMU_TRANSLATE);
  rtl_fsr(s, ddest, ddest, FPCALL_W32);
}

def_EHelper(fsw_mmu) {
  rtl_funbox(s, s0, ddest);
  rtl_sm(s, s0, dsrc1, id_src2->imm, 4, MMU_TRANSLATE);
}

#define def_fop_template(name, op, w) \
  def_EHelper(name) { \
    rtl_hostcall(s, HOSTCALL_FP, ddest, dsrc1, dsrc2, FPCALL_CMD(op, w)); \
    rtl_fsr(s, ddest, ddest, w); \
  }

def_fop_template(fadds, FPCALL_ADD, FPCALL_W32)
def_fop_template(fsubs, FPCALL_SUB, FPCALL_W32)
def_fop_template(fmuls, FPCALL_MUL, FPCALL_W32)
def_fop_template(fdivs, FPCALL_DIV, FPCALL_W32)
def_fop_template(fmins, FPCALL_MIN, FPCALL_W32)
def_fop_template(fmaxs, FPCALL_MAX, FPCALL_W32)

def_EHelper(fsqrts) {
  rtl_hostcall(s, HOSTCALL_FP, ddest, dsrc1, rz, FPCALL_CMD(FPCALL_SQRT, FPCALL_W32));
  rtl_fsr(s, ddest, ddest, FPCALL_W32);
}

def_EHelper(fmadds) {
  rtl_mv(s, s0, &fpreg_l(s->isa.instr.fp.funct5)); // rs3
  rtl_hostcall(s, HOSTCALL_FP, s0, dsrc1, dsrc2, FPCALL_CMD(FPCALL_MADD, FPCALL_W32));
  rtl_fsr(s, ddest, s0, FPCALL_W32);
}

def_EHelper(fmsubs) {
  rtl_mv(s, s0, &fpreg_l(s->isa.instr.fp.funct5)); // rs3
  rtl_xori(s, s0, s0, F32_SIGN);
  rtl_hostcall(s, HOSTCALL_FP, s0, dsrc1, dsrc2, FPCALL_CMD(FPCALL_MADD, FPCALL_W32));
  rtl_fsr(s, ddest, s0, FPCALL_W32);
}

def_EHelper(fnmsubs) {
  rtl_mv(s, s0, &fpreg_l(s->isa.instr.fp.funct5)); // rs3
  rtl_xori(s, s1, dsrc1, F32_SIGN);
  rtl_hostcall(s, HOSTCALL_FP, s0, s1, dsrc2, FPCALL_CMD(FPCALL_MADD, FPCALL_W32));
  rtl_fsr(s, ddest, s0, FPCALL_W32);
}

def_EHelper(fnmadds) {
  rtl_mv(s, s0, &fpreg_l(s->isa.instr.fp.funct5)); // rs3
  rtl_xori(s, s1, dsrc1, F32_SIGN);
  rtl_xori(s, s0, s0, F32_SIGN);
  rtl_hostcall(s, HOSTCALL_FP, s0, s1, dsrc2, FPCALL_CMD(FPCALL_MADD, FPCALL_W32));
  rtl_fsr(s, ddest, s0, FPCALL_W32);
}

def_EHelper(fles) {
  rtl_hostcall(s, HOSTCALL_FP, ddest, dsrc1, dsrc2, FPCALL_CMD(FPCALL_LE, FPCALL_W32));
}

def_EHelper(flts) {
  rtl_mv(s, ddest, dsrc1);
  rtl_hostcall(s, HOSTCALL_FP, ddest, dsrc1, dsrc2, FPCALL_CMD(FPCALL_LT, FPCALL_W32));
}

def_EHelper(feqs) {
  rtl_mv(s, ddest, dsrc1);
  rtl_hostcall(s, HOSTCALL_FP, ddest, dsrc1, dsrc2, FPCALL_CMD(FPCALL_EQ, FPCALL_W32));
}

def_EHelper(fcvt_s_w) {
  rtl_hostcall(s, HOSTCALL_FP, ddest, dsrc1, rz, FPCALL_CMD(FPCALL_I32ToF, FPCALL_W32));
  rtl_fsr(s, ddest, ddest, FPCALL_W32);
}

def_EHelper(fcvt_s_wu) {
  rtl_hostcall(s, HOSTCALL_FP, ddest, dsrc1, rz, FPCALL_CMD(FPCALL_U32ToF, FPCALL_W32));
  rtl_fsr(s, ddest, ddest, FPCALL_W32);
}

def_EHelper(fcvt_s_l) {
  rtl_hostcall(s, HOSTCALL_FP, ddest, dsrc1, rz, FPCALL_CMD(FPCALL_I64ToF, FPCALL_W32));
  rtl_fsr(s, ddest, ddest, FPCALL_W32);
}

def_EHelper(fcvt_s_lu) {
  rtl_hostcall(s, HOSTCALL_FP, ddest, dsrc1, rz, FPCALL_CMD(FPCALL_U64ToF, FPCALL_W32));
  rtl_fsr(s, ddest, ddest, FPCALL_W32);
}

def_EHelper(fcvt_w_s) {
  rtl_hostcall(s, HOSTCALL_FP, ddest, dsrc1, rz, FPCALL_CMD(FPCALL_FToI32, FPCALL_W32));
  rtl_sext(s, ddest, ddest, 4);
}

def_EHelper(fcvt_wu_s) {
  rtl_hostcall(s, HOSTCALL_FP, ddest, dsrc1, rz, FPCALL_CMD(FPCALL_FToU32, FPCALL_W32));
  rtl_sext(s, ddest, ddest, 4);
}

def_EHelper(fcvt_l_s) {
  rtl_hostcall(s, HOSTCALL_FP, ddest, dsrc1, rz, FPCALL_CMD(FPCALL_FToI64, FPCALL_W32));
}

def_EHelper(fcvt_lu_s) {
  rtl_hostcall(s, HOSTCALL_FP, ddest, dsrc1, rz, FPCALL_CMD(FPCALL_FToU64, FPCALL_W32));
}

def_EHelper(fsgnjs) {
  rtl_funbox(s, s0, dsrc1);
  rtl_funbox(s, ddest, dsrc2);
  rtl_andi(s, s0, s0, ~F32_SIGN);
  rtl_andi(s, ddest, ddest, F32_SIGN);
  rtl_or(s, ddest, s0, ddest);
  rtl_fsr(s, ddest, ddest, FPCALL_W32);
}

def_EHelper(fsgnjns) {
  rtl_funbox(s, s0, dsrc1);
  rtl_funbox(s, ddest, dsrc2);
  rtl_andi(s, s0, s0, ~F32_SIGN);
  rtl_xori(s, ddest, ddest, F32_SIGN);
  rtl_andi(s, ddest, ddest, F32_SIGN);
  rtl_or(s, ddest, s0, ddest);
  rtl_fsr(s, ddest, ddest, FPCALL_W32);
}

def_EHelper(fsgnjxs) {
  rtl_funbox(s, s0, dsrc1);
  rtl_funbox(s, ddest, dsrc2);
  rtl_xor(s, ddest, s0, ddest);
  rtl_andi(s, s0, s0, ~F32_SIGN);
  rtl_andi(s, ddest, ddest, F32_SIGN);
  rtl_or(s, ddest, s0, ddest);
  rtl_fsr(s, ddest, ddest, FPCALL_W32);
}

def_EHelper(fmv_x_w) {
  rtl_sext(s, ddest, dsrc1, 4);
}

def_EHelper(fmv_w_x) {
  rtl_fsr(s, ddest, dsrc1, FPCALL_W32);
}

def_EHelper(fclasss) {
  rtl_fclass(s, ddest, dsrc1, FPCALL_W32);
}
