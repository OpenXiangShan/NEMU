// standard pseudo instructions
// See Chapter 25 "RISC-V Assembly Programmer's Handbook" in the RISC-V ISA manual
//
// The following pseudo instructions are excluded
// (1) seem not frequently present during execution
//       nop    not    neg    negw
//       seqz   snez   sltz   sgtz
//       [[all CSR instructions]]
// (2) only expansion without optimization
//       la
//       l{b|h|w|d} symbol
//       s{b|h|w|d} symbol
//       bgt    ble    bgtu   bleu
//       call   tail
//       fence
// (3) still not considered
//       fmv.s  fabs.s fneg.s
//       fmv.d  fabs.d fneg.d
//       fl{w|d} symbol
//       fs{w|d} symbol
// (4) already provided in RVC
//       j      jal(ra)   jalr(rs)
//       beqz   bnez
//       li     mv

def_EHelper(p_sext_w) { rtl_addiw(s, ddest, dsrc1, 0); }
def_EHelper(p_blez) { rtl_jrelop(s, RELOP_GE, rz, dsrc2, id_dest->imm); }
def_EHelper(p_bgtz) { rtl_jrelop(s, RELOP_LT, rz, dsrc2, id_dest->imm); }
def_EHelper(p_bltz) { rtl_jrelop(s, RELOP_LT, dsrc1, rz, id_dest->imm); }
def_EHelper(p_bgez) { rtl_jrelop(s, RELOP_GE, dsrc1, rz, id_dest->imm); }

def_EHelper(p_jal) {
  rtl_li(s, &gpr(1), id_src2->imm);
  ftrace_call(s->pc, id_src1->imm);
  rtl_j(s, id_src1->imm);
}

def_EHelper(p_jalr_ra) {
  rtl_addi(s, s0, &gpr(1), id_src2->imm);
  rtl_li(s, &gpr(1), s->snpc);
  ftrace_call(s->pc, *s0);
  rtl_jr(s, s0);
}

def_EHelper(p_jalr_ra_noimm) {
  rtl_mv(s, s0, &gpr(1));
  rtl_li(s, &gpr(1), s->snpc);
  ftrace_call(s->pc, *s0);
  rtl_jr(s, s0);
}

def_EHelper(p_jalr_t0) {
  rtl_addi(s, s0, &gpr(6), id_src2->imm);
  rtl_jr(s, s0);
}

def_EHelper(p_ret) {
//  IFDEF(CONFIG_ENGINE_INTERPRETER, rtl_andi(s, s0, s0, ~0x1u));
  ftrace_ret(s->pc);
  rtl_jr(s, &gpr(1));
}

// non-standard pseudo instructions

def_EHelper(p_li_0) { rtl_li(s, ddest, 0); }
def_EHelper(p_li_1) { rtl_li(s, ddest, 1); }
def_EHelper(p_inc) { rtl_addi(s, ddest, ddest, 1); }
def_EHelper(p_dec) { rtl_subi(s, ddest, ddest, 1); }
def_EHelper(p_mv_src1) { rtl_mv(s, ddest, dsrc1); }
def_EHelper(p_mv_src2) { rtl_mv(s, ddest, dsrc2); }
def_EHelper(p_not) { rtl_not(s, ddest, dsrc1); }
def_EHelper(p_neg) { rtl_neg(s, ddest, dsrc2); }
def_EHelper(p_negw) { rtl_subw(s, ddest, rz, dsrc2); }
def_EHelper(p_seqz) { rtl_setrelopi(s, RELOP_LTU, ddest, dsrc1, 1); }
def_EHelper(p_snez) { rtl_setrelop(s, RELOP_LTU, ddest, rz, dsrc2); }
def_EHelper(p_sltz) { rtl_setrelop(s, RELOP_LT, ddest, dsrc1, rz); }
def_EHelper(p_sgtz) { rtl_setrelop(s, RELOP_LT, ddest, rz, dsrc2); }

#define F32_SIGN ((uint64_t)1 << 31)
#define F64_SIGN ((uint64_t)1 << 63)
#define LOW32_MASK (((uint64_t)1 << 32) - 1)

def_EHelper(p_fmv_s) { rtl_andi(s, ddest, dsrc1, LOW32_MASK); }
def_EHelper(p_fabs_s) { rtl_andi(s, ddest, dsrc1, ~F32_SIGN); }
def_EHelper(p_fneg_s) { rtl_xori(s, ddest, dsrc1, F32_SIGN); }
def_EHelper(p_fmv_d) { rtl_mv(s, ddest, dsrc1); }
def_EHelper(p_fabs_d) { rtl_andi(s, ddest, dsrc1, ~F64_SIGN); }
def_EHelper(p_fneg_d) { rtl_xori(s, ddest, dsrc1, F64_SIGN); }
def_EHelper(p_li_ra) { rtl_li(s, &gpr(1), id_src2->imm); }
def_EHelper(p_li_t0) { rtl_li(s, &gpr(6), id_src2->imm); }
