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
  extern void trace_write_arthi_src(uint64_t, uint64_t);
  trace_write_arthi_src(*dsrc1, 0);

  rtl_hostcall(s, HOSTCALL_FP, ddest, dsrc1, rz, FPCALL_CMD(FPCALL_SQRT, FPCALL_W64));
  rtl_fsr(s, ddest, ddest, FPCALL_W64);
}

def_EHelper(fmaddd) {
  extern void trace_write_arthi_src(uint64_t, uint64_t);
  trace_write_arthi_src(*dsrc1, *dsrc2);

  rtl_mv(s, s0, &fpreg_l(s->isa.instr.fp.funct5)); // rs3
  rtl_hostcall(s, HOSTCALL_FP, s0, dsrc1, dsrc2, FPCALL_CMD(FPCALL_MADD, FPCALL_W64));
  rtl_fsr(s, ddest, s0, FPCALL_W64);
}

def_EHelper(fmsubd) {
  extern void trace_write_arthi_src(uint64_t, uint64_t);
  trace_write_arthi_src(*dsrc1, *dsrc2);

  rtl_mv(s, s0, &fpreg_l(s->isa.instr.fp.funct5)); // rs3
  rtl_xori(s, s0, s0, F64_SIGN);
  rtl_hostcall(s, HOSTCALL_FP, s0, dsrc1, dsrc2, FPCALL_CMD(FPCALL_MADD, FPCALL_W64));
  rtl_fsr(s, ddest, s0, FPCALL_W64);
}

def_EHelper(fnmsubd) {
  extern void trace_write_arthi_src(uint64_t, uint64_t);
  trace_write_arthi_src(*dsrc1, *dsrc2);

  rtl_mv(s, s0, &fpreg_l(s->isa.instr.fp.funct5)); // rs3
  rtl_xori(s, s1, dsrc1, F64_SIGN);
  rtl_hostcall(s, HOSTCALL_FP, s0, s1, dsrc2, FPCALL_CMD(FPCALL_MADD, FPCALL_W64));
  rtl_fsr(s, ddest, s0, FPCALL_W64);
}

def_EHelper(fnmaddd) {
  extern void trace_write_arthi_src(uint64_t, uint64_t);
  trace_write_arthi_src(*dsrc1, *dsrc2);

  rtl_mv(s, s0, &fpreg_l(s->isa.instr.fp.funct5)); // rs3
  rtl_xori(s, s1, dsrc1, F64_SIGN);
  rtl_xori(s, s0, s0, F64_SIGN);
  rtl_hostcall(s, HOSTCALL_FP, s0, s1, dsrc2, FPCALL_CMD(FPCALL_MADD, FPCALL_W64));
  rtl_fsr(s, ddest, s0, FPCALL_W64);
}

def_EHelper(fled) {
  extern void trace_write_arthi_src(uint64_t, uint64_t);
  trace_write_arthi_src(*dsrc1, *dsrc2);

  rtl_hostcall(s, HOSTCALL_FP, ddest, dsrc1, dsrc2, FPCALL_CMD(FPCALL_LE, FPCALL_W64));
}

def_EHelper(fltd) {
  extern void trace_write_arthi_src(uint64_t, uint64_t);
  trace_write_arthi_src(*dsrc1, *dsrc2);

  rtl_hostcall(s, HOSTCALL_FP, ddest, dsrc1, dsrc2, FPCALL_CMD(FPCALL_LT, FPCALL_W64));
}

def_EHelper(feqd) {
  extern void trace_write_arthi_src(uint64_t, uint64_t);
  trace_write_arthi_src(*dsrc1, *dsrc2);

  rtl_hostcall(s, HOSTCALL_FP, ddest, dsrc1, dsrc2, FPCALL_CMD(FPCALL_EQ, FPCALL_W64));
}

def_EHelper(fcvt_d_w) {
  extern void trace_write_arthi_src(uint64_t, uint64_t);
  trace_write_arthi_src(*dsrc1, 0);

  rtl_hostcall(s, HOSTCALL_FP, ddest, dsrc1, rz, FPCALL_CMD(FPCALL_I32ToF, FPCALL_W64));
  rtl_fsr(s, ddest, ddest, FPCALL_W64);
}

def_EHelper(fcvt_d_wu) {
  extern void trace_write_arthi_src(uint64_t, uint64_t);
  trace_write_arthi_src(*dsrc1, 0);

  rtl_hostcall(s, HOSTCALL_FP, ddest, dsrc1, rz, FPCALL_CMD(FPCALL_U32ToF, FPCALL_W64));
  rtl_fsr(s, ddest, ddest, FPCALL_W64);
}

def_EHelper(fcvt_d_l) {
  extern void trace_write_arthi_src(uint64_t, uint64_t);
  trace_write_arthi_src(*dsrc1, 0);

  rtl_hostcall(s, HOSTCALL_FP, ddest, dsrc1, rz, FPCALL_CMD(FPCALL_I64ToF, FPCALL_W64));
  rtl_fsr(s, ddest, ddest, FPCALL_W64);
}

def_EHelper(fcvt_d_lu) {
  extern void trace_write_arthi_src(uint64_t, uint64_t);
  trace_write_arthi_src(*dsrc1, 0);

  rtl_hostcall(s, HOSTCALL_FP, ddest, dsrc1, rz, FPCALL_CMD(FPCALL_U64ToF, FPCALL_W64));
  rtl_fsr(s, ddest, ddest, FPCALL_W64);
}

def_EHelper(fcvt_w_d) {
  extern void trace_write_arthi_src(uint64_t, uint64_t);
  trace_write_arthi_src(*dsrc1, 0);

  rtl_hostcall(s, HOSTCALL_FP, ddest, dsrc1, rz, FPCALL_CMD(FPCALL_FToI32, FPCALL_W64));
  rtl_sext(s, ddest, ddest, 4);
}

def_EHelper(fcvt_wu_d) {
  extern void trace_write_arthi_src(uint64_t, uint64_t);
  trace_write_arthi_src(*dsrc1, 0);

  rtl_hostcall(s, HOSTCALL_FP, ddest, dsrc1, rz, FPCALL_CMD(FPCALL_FToU32, FPCALL_W64));
  rtl_sext(s, ddest, ddest, 4);
}

def_EHelper(fcvt_l_d) {
  extern void trace_write_arthi_src(uint64_t, uint64_t);
  trace_write_arthi_src(*dsrc1, 0);

  rtl_hostcall(s, HOSTCALL_FP, ddest, dsrc1, rz, FPCALL_CMD(FPCALL_FToI64, FPCALL_W64));
}

def_EHelper(fcvt_lu_d) {
  extern void trace_write_arthi_src(uint64_t, uint64_t);
  trace_write_arthi_src(*dsrc1, 0);

  rtl_hostcall(s, HOSTCALL_FP, ddest, dsrc1, rz, FPCALL_CMD(FPCALL_FToU64, FPCALL_W64));
}

def_EHelper(fcvt_d_s) {
  extern void trace_write_arthi_src(uint64_t, uint64_t);
  trace_write_arthi_src(*dsrc1, 0);

  rtl_hostcall(s, HOSTCALL_FP, ddest, dsrc1, rz, FPCALL_CMD(FPCALL_F32ToF64, FPCALL_W64));
  rtl_fsr(s, ddest, ddest, FPCALL_W64);
}

def_EHelper(fcvt_s_d) {
  extern void trace_write_arthi_src(uint64_t, uint64_t);
  trace_write_arthi_src(*dsrc1, 0);

  rtl_hostcall(s, HOSTCALL_FP, ddest, dsrc1, rz, FPCALL_CMD(FPCALL_F64ToF32, FPCALL_W64));
  rtl_fsr(s, ddest, ddest, FPCALL_W32);
}

def_EHelper(fsgnjd) {
  extern void trace_write_arthi_src(uint64_t, uint64_t);
  trace_write_arthi_src(*dsrc1, *dsrc2);

  rtl_andi(s, s0, dsrc1, ~F64_SIGN);
  rtl_andi(s, ddest, dsrc2, F64_SIGN);
  rtl_or(s, ddest, s0, ddest);
  rtl_fsr(s, ddest, ddest, FPCALL_W64);
}

def_EHelper(fsgnjnd) {
  extern void trace_write_arthi_src(uint64_t, uint64_t);
  trace_write_arthi_src(*dsrc1, *dsrc2);

  rtl_andi(s, s0, dsrc1, ~F64_SIGN);
  rtl_xori(s, ddest, dsrc2, F64_SIGN);
  rtl_andi(s, ddest, ddest, F64_SIGN);
  rtl_or(s, ddest, s0, ddest);
  rtl_fsr(s, ddest, ddest, FPCALL_W64);
}

def_EHelper(fsgnjxd) {
  extern void trace_write_arthi_src(uint64_t, uint64_t);
  trace_write_arthi_src(*dsrc1, *dsrc2);

  rtl_andi(s, s0, dsrc1, ~F64_SIGN);
  rtl_xor(s, ddest, dsrc1, dsrc2);
  rtl_andi(s, ddest, ddest, F64_SIGN);
  rtl_or(s, ddest, s0, ddest);
  rtl_fsr(s, ddest, ddest, FPCALL_W64);
}

def_EHelper(fmv_x_d) {
  extern void trace_write_arthi_src(uint64_t, uint64_t);
  trace_write_arthi_src(*dsrc1, 0);

  rtl_sext(s, ddest, dsrc1, 8);
}

def_EHelper(fmv_d_x) {
  extern void trace_write_arthi_src(uint64_t, uint64_t);
  trace_write_arthi_src(*dsrc1, 0);

  rtl_fsr(s, ddest, dsrc1, FPCALL_W64);
}

def_EHelper(fclassd) {
  extern void trace_write_arthi_src(uint64_t, uint64_t);
  trace_write_arthi_src(*dsrc1, 0);

  rtl_fclass(s, ddest, dsrc1, FPCALL_W64);
}
