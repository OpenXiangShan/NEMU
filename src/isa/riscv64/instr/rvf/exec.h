/***************************************************************************************
* Copyright (c) 2014-2021 Zihao Yu, Nanjing University
* Copyright (c) 2020-2022 Institute of Computing Technology, Chinese Academy of Sciences
*
* NEMU is licensed under Mulan PSL v2.
* You can use this software according to the terms and conditions of the Mulan PSL v2.
* You may obtain a copy of Mulan PSL v2 at:
*          http://license.coscl.org.cn/MulanPSL2
*
* THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND,
* EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT,
* MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE.
*
* See the Mulan PSL v2 for more details.
***************************************************************************************/

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
  rtl_sm(s, ddest, dsrc1, id_src2->imm, 4, MMU_TRANSLATE);
}

#define def_fop_template(name, op, w) \
  def_EHelper(name) { \
    rtl_hostcall(s, HOSTCALL_FP, ddest, dsrc1, dsrc2, FPCALL_CMD(op, w)); \
    rtl_fsr(s, ddest, ddest, w); \
  }

def_EHelper(fadds) {
  rtl_f32_add(s, ddest, dsrc1, dsrc2);
  rtl_fsr(s, ddest, ddest, FPCALL_W32);
}

def_EHelper(fsubs) {
  rtl_f32_sub(s, ddest, dsrc1, dsrc2);
  rtl_fsr(s, ddest, ddest, FPCALL_W32);
}

def_EHelper(fdivs) {
  rtl_f32_div(s, ddest, dsrc1, dsrc2);
  rtl_fsr(s, ddest, ddest, FPCALL_W32);
}

def_EHelper(fmins) {
  rtl_f32_min(s, ddest, dsrc1, dsrc2);
  rtl_fsr(s, ddest, ddest, FPCALL_W32);
}

def_EHelper(fmaxs) {
  rtl_f32_max(s, ddest, dsrc1, dsrc2);
  rtl_fsr(s, ddest, ddest, FPCALL_W32);
}

def_EHelper(fmuls) {
  rtl_f32_mul(s, ddest, dsrc1, dsrc2);
  rtl_fsr(s, ddest, ddest, FPCALL_W32);
}

def_EHelper(fsqrts) {
  rtl_f32_sqrt(s, ddest, dsrc1);
  rtl_fsr(s, ddest, ddest, FPCALL_W32);
}

def_EHelper(fmadds) {
  rtl_mv(s, s0, &fpreg_l(s->isa.instr.fp.funct5)); // rs3
  rtl_f32_madd(s, s0, dsrc1, dsrc2);
  rtl_fsr(s, ddest, s0, FPCALL_W32);
}

def_EHelper(fmsubs) {
  rtl_mv(s, s0, &fpreg_l(s->isa.instr.fp.funct5)); // rs3
  rtl_xori(s, s0, s0, F32_SIGN);
  rtl_f32_madd(s, s0, dsrc1, dsrc2);
  rtl_fsr(s, ddest, s0, FPCALL_W32);
}

def_EHelper(fnmsubs) {
  rtl_mv(s, s0, &fpreg_l(s->isa.instr.fp.funct5)); // rs3
  rtl_xori(s, s1, dsrc1, F32_SIGN);
  rtl_f32_madd(s, s0, s1, dsrc2);
  rtl_fsr(s, ddest, s0, FPCALL_W32);
}

def_EHelper(fnmadds) {
  rtl_mv(s, s0, &fpreg_l(s->isa.instr.fp.funct5)); // rs3
  rtl_xori(s, s1, dsrc1, F32_SIGN);
  rtl_xori(s, s0, s0, F32_SIGN);
  rtl_f32_madd(s, s0, s1, dsrc2);
  rtl_fsr(s, ddest, s0, FPCALL_W32);
}

def_EHelper(fles) {
  rtl_f32_le(s, ddest, dsrc1, dsrc2);
  void fp_set_dirty();
  fp_set_dirty();
}

def_EHelper(flts) {
  rtl_f32_lt(s, ddest, dsrc1, dsrc2);
  void fp_set_dirty();
  fp_set_dirty();
}

def_EHelper(feqs) {
  rtl_f32_eq(s, ddest, dsrc1, dsrc2);
  void fp_set_dirty();
  fp_set_dirty();
}

def_EHelper(fcvt_s_w) {
  rtl_f32_from_i32(s, ddest, dsrc1);
  rtl_fsr(s, ddest, ddest, FPCALL_W32);
}

def_EHelper(fcvt_s_wu) {
  rtl_f32_from_ui32(s, ddest, dsrc1);
  rtl_fsr(s, ddest, ddest, FPCALL_W32);
}

def_EHelper(fcvt_s_l) {
  rtl_f32_from_i64(s, ddest, dsrc1);
  rtl_fsr(s, ddest, ddest, FPCALL_W32);
}

def_EHelper(fcvt_s_lu) {
  rtl_f32_from_ui64(s, ddest, dsrc1);
  rtl_fsr(s, ddest, ddest, FPCALL_W32);
}

def_EHelper(fcvt_w_s) {
  rtl_f32_to_i32_r(s, ddest, dsrc1);
  rtl_sext(s, ddest, ddest, 4);
  void fp_set_dirty();
  fp_set_dirty();
}

def_EHelper(fcvt_wu_s) {
  rtl_f32_to_ui32_r(s, ddest, dsrc1);
  rtl_sext(s, ddest, ddest, 4);
  void fp_set_dirty();
  fp_set_dirty();
}

def_EHelper(fcvt_l_s) {
  rtl_f32_to_i64_r(s, ddest, dsrc1);
  void fp_set_dirty();
  fp_set_dirty();
}

def_EHelper(fcvt_lu_s) {
  rtl_f32_to_ui64_r(s, ddest, dsrc1);
  void fp_set_dirty();
  fp_set_dirty();
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
