#define F32_SIGN ((uint64_t)1 << 31)

def_EHelper(flw) {
  rtl_lm(s, ddest, dsrc1, id_src2->imm, 4, MMU_DIRECT);
  rtl_fsr(s, ddest, ddest, FPCALL_W32);
}

def_EHelper(fsw) {
  rtl_funbox(s, s0, dsrc2);
  rtl_sm(s, s0, dsrc1, id_dest->imm, 4, MMU_DIRECT);
}

def_EHelper(flw_mmu) {
  rtl_lm(s, ddest, dsrc1, id_src2->imm, 4, MMU_TRANSLATE);
  rtl_fsr(s, ddest, ddest, FPCALL_W32);
}

def_EHelper(fsw_mmu) {
  rtl_funbox(s, s0, dsrc2);
  rtl_sm(s, s0, dsrc1, id_dest->imm, 4, MMU_TRANSLATE);
}

#define check_rm(s) IFDEF(CONFIG_RT_CHECK, Assert(INSTR_FP_RM(s) == 0b111, "s->pc = " FMT_WORD, s->pc));

#define def_fop_template(name, w, has_rm) \
  def_EHelper(name) { \
    if (has_rm) { check_rm(s); } \
    concat(rtl_, name)(s, ddest, dsrc1, dsrc2); \
    rtl_fsr(s, ddest, ddest, w); \
  }

def_fop_template(fadds, FPCALL_W32, true)
def_fop_template(fsubs, FPCALL_W32, true)
def_fop_template(fmuls, FPCALL_W32, true)
def_fop_template(fdivs, FPCALL_W32, true)
def_fop_template(fmins, FPCALL_W32, false)
def_fop_template(fmaxs, FPCALL_W32, false)

def_EHelper(fsqrts) {
  check_rm(s);
  rtl_fsqrts(s, ddest, dsrc1);
  rtl_fsr(s, ddest, ddest, FPCALL_W32);
}

def_EHelper(fmadds) {
  check_rm(s);
  rtl_mv(s, s0, &fpr(INSTR_RS3(s)));
  rtl_fmadds(s, s0, dsrc1, dsrc2);
  rtl_fsr(s, ddest, s0, FPCALL_W32);
}

def_EHelper(fmsubs) {
  check_rm(s);
  rtl_mv(s, s0, &fpr(INSTR_RS3(s)));
  rtl_xori(s, s0, s0, F32_SIGN);
  rtl_fmadds(s, s0, dsrc1, dsrc2);
  rtl_fsr(s, ddest, s0, FPCALL_W32);
}

def_EHelper(fnmsubs) {
  check_rm(s);
  rtl_mv(s, s0, &fpr(INSTR_RS3(s)));
  rtl_xori(s, s1, dsrc1, F32_SIGN);
  rtl_fmadds(s, s0, s1, dsrc2);
  rtl_fsr(s, ddest, s0, FPCALL_W32);
}

def_EHelper(fnmadds) {
  check_rm(s);
  rtl_mv(s, s0, &fpr(INSTR_RS3(s)));
  rtl_fmadds(s, s0, dsrc1, dsrc2);
  rtl_xori(s, s0, s0, F32_SIGN);
  rtl_fsr(s, ddest, s0, FPCALL_W32);
}

def_EHelper(fles) {
  rtl_fles(s, ddest, dsrc1, dsrc2);
}

def_EHelper(flts) {
  rtl_flts(s, ddest, dsrc1, dsrc2);
}

def_EHelper(feqs) {
  rtl_feqs(s, ddest, dsrc1, dsrc2);
}

def_EHelper(fcvt_s_w) {
  check_rm(s);
  rtl_fcvt_i32_to_f32(s, ddest, dsrc1);
  rtl_fsr(s, ddest, ddest, FPCALL_W32);
}

def_EHelper(fcvt_s_wu) {
  check_rm(s);
  rtl_fcvt_u32_to_f32(s, ddest, dsrc1);
  rtl_fsr(s, ddest, ddest, FPCALL_W32);
}

def_EHelper(fcvt_s_l) {
  check_rm(s);
  rtl_fcvt_i64_to_f32(s, ddest, dsrc1);
  rtl_fsr(s, ddest, ddest, FPCALL_W32);
}

def_EHelper(fcvt_s_lu) {
  check_rm(s);
  rtl_fcvt_u64_to_f32(s, ddest, dsrc1);
  rtl_fsr(s, ddest, ddest, FPCALL_W32);
}

def_EHelper(fcvt_w_s) {
  check_rm(s);
  rtl_fcvt_f32_to_i32(s, ddest, dsrc1);
  rtl_sext(s, ddest, ddest, 4);
}

def_EHelper(fcvt_wu_s) {
  check_rm(s);
  rtl_fcvt_f32_to_u32(s, ddest, dsrc1);
  rtl_sext(s, ddest, ddest, 4);
}

def_EHelper(fcvt_l_s) {
  check_rm(s);
  rtl_fcvt_f32_to_i64(s, ddest, dsrc1);
}

def_EHelper(fcvt_lu_s) {
  check_rm(s);
  rtl_fcvt_f32_to_u64(s, ddest, dsrc1);
}

def_EHelper(fsgnjs) {
  rtl_andi(s, s0, dsrc1, ~F32_SIGN);
  rtl_andi(s, ddest, dsrc2, F32_SIGN);
  rtl_or(s, ddest, s0, ddest);
}

def_EHelper(fsgnjns) {
  rtl_andi(s, s0, dsrc1, ~F32_SIGN);
  rtl_xori(s, ddest, dsrc2, F32_SIGN);
  rtl_andi(s, ddest, ddest, F32_SIGN);
  rtl_or(s, ddest, s0, ddest);
}

def_EHelper(fsgnjxs) {
  rtl_andi(s, s0, dsrc1, ~F32_SIGN);
  rtl_xor(s, ddest, dsrc1, dsrc2);
  rtl_andi(s, ddest, ddest, F32_SIGN);
  rtl_or(s, ddest, s0, ddest);
}

def_EHelper(fmv_x_w) {
  rtl_sext(s, ddest, dsrc1, 4);
}

def_EHelper(fmv_w_x) {
  rtl_fsr(s, ddest, dsrc1, FPCALL_W32);
}


def_EHelper(fcvt_s_w_rm) {
  rtl_fpcall(s, FPCALL_SETRM_CONST, NULL, NULL, NULL, INSTR_FP_RM(s));
  rtl_fcvt_i32_to_f32(s, ddest, dsrc1);
  rtl_fpcall(s, FPCALL_SETRM_CONST, NULL, NULL, NULL, 0b111);
  rtl_fsr(s, ddest, ddest, FPCALL_W32);
}

def_EHelper(fcvt_w_s_rm) {
  rtl_fpcall(s, FPCALL_SETRM_CONST, NULL, NULL, NULL, INSTR_FP_RM(s));
  rtl_fcvt_f32_to_i32(s, ddest, dsrc1);
  rtl_fpcall(s, FPCALL_SETRM_CONST, NULL, NULL, NULL, 0b111);
  rtl_sext(s, ddest, ddest, 4);
}

def_EHelper(fcvt_l_s_rm) {
  rtl_fpcall(s, FPCALL_SETRM_CONST, NULL, NULL, NULL, INSTR_FP_RM(s));
  rtl_fcvt_f32_to_i64(s, ddest, dsrc1);
  rtl_fpcall(s, FPCALL_SETRM_CONST, NULL, NULL, NULL, 0b111);
}

def_EHelper(fcvt_lu_s_rm) {
  rtl_fpcall(s, FPCALL_SETRM_CONST, NULL, NULL, NULL, INSTR_FP_RM(s));
  rtl_fcvt_f32_to_u64(s, ddest, dsrc1);
  rtl_fpcall(s, FPCALL_SETRM_CONST, NULL, NULL, NULL, 0b111);
}
