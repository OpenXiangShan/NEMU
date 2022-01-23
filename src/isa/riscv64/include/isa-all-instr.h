#include <cpu/decode.h>
#include "../local-include/rtl.h"

#ifndef __ICS_EXPORT

#define INSTR_RVI(f) \
  f(lui) f(auipc) f(jal) \
  f(ld) f(lw) f(lh) f(lb) f(lwu) f(lhu) f(lbu) f(sd) f(sw) f(sh) f(sb) \
  f(ld_mmu) f(lw_mmu) f(lh_mmu) f(lb_mmu) f(lwu_mmu) f(lhu_mmu) f(lbu_mmu) \
  f(sd_mmu) f(sw_mmu) f(sh_mmu) f(sb_mmu) \
  f(ldsp) f(lwsp) f(sdsp) f(swsp) f(ldsp_mmu) f(lwsp_mmu) f(sdsp_mmu) f(swsp_mmu) \
  f(add) f(sll) f(srl) f(slt) f(sltu) f(xor) f(or) f(sub) f(sra) f(and) \
  f(addi) f(slli) f(srli) f(slti) f(sltiu) f(xori) f(ori) f(srai) f(andi) \
  f(jalr) f(beq) f(bne) f(blt) f(bge) f(bltu) f(bgeu) \
  f(addw) f(sllw) f(srlw) f(subw) f(sraw) \
  f(addiw) f(slliw) f(srliw) f(sraiw) \
  f(fence_i) f(fence) \
  f(p_ret) f(jal_next) \
  f(beq_tnext) f(bne_tnext) f(blt_tnext) f(bge_tnext) f(bltu_tnext) f(bgeu_tnext) \
  f(beq_ntnext) f(bne_ntnext) f(blt_ntnext) f(bge_ntnext) f(bltu_ntnext) f(bgeu_ntnext) \

#define INSTR_RVM(f) \
  f(mul) f(mulh) f(mulhu) f(mulhsu) f(div) f(divu) f(rem) f(remu) \
  f(mulw) f(divw) f(divuw) f(remw) f(remuw) \

#define INSTR_RVF(f) \
  f(flw) f(fsw) f(flw_mmu) f(fsw_mmu) \
  f(flwsp) f(flwsp_mmu) f(fswsp) f(fswsp_mmu) \
  f(fadds) f(fsubs) f(fmuls) f(fdivs) f(fmins) f(fmaxs) \
  f(fsqrts) f(fles) f(flts) f(feqs) \
  f(fmadds) f(fmsubs) f(fnmsubs) f(fnmadds) \
  f(fsgnjs) f(fsgnjns) f(fsgnjxs) \
  f(fcvt_s_w) f(fcvt_s_wu) f(fcvt_s_l) f(fcvt_s_lu) \
  f(fcvt_w_s) f(fcvt_wu_s) f(fcvt_l_s) f(fcvt_lu_s) \
  f(fmv_x_w) f(fmv_w_x) \
  f(fcvt_s_w_rm) \
  f(fcvt_w_s_rm) f(fcvt_l_s_rm) f(fcvt_lu_s_rm) \
  f(fcvt_w_s_rmm) f(fcvt_wu_s_rmm) f(fcvt_l_s_rmm) f(fcvt_lu_s_rmm) \
  f(fcvt_wu_s_rm) \

#define INSTR_RVD(f) \
  f(fld) f(fsd) f(fld_mmu) f(fsd_mmu) \
  f(fldsp) f(fldsp_mmu) f(fsdsp) f(fsdsp_mmu) \
  f(faddd) f(fsubd) f(fmuld) f(fdivd) f(fmind) f(fmaxd) \
  f(fsqrtd) f(fled) f(fltd) f(feqd) \
  f(fmaddd) f(fmsubd) f(fnmsubd) f(fnmaddd) \
  f(fsgnjd) f(fsgnjnd) f(fsgnjxd) \
  f(fcvt_d_w) f(fcvt_d_wu) f(fcvt_d_l) f(fcvt_d_lu) \
  f(fcvt_w_d) f(fcvt_wu_d) f(fcvt_l_d) f(fcvt_lu_d) \
  f(fcvt_d_w_rm) f(fcvt_d_wu_rm) f(fcvt_d_l_rm) \
  f(fcvt_w_d_rm) f(fcvt_wu_d_rm) f(fcvt_l_d_rm) f(fcvt_lu_d_rm) \
  f(fcvt_d_s_rm) f(fcvt_s_d_rm) \
  f(fmv_x_d) f(fmv_d_x) \
  f(fcvt_d_s) f(fcvt_s_d) \
  f(fclassd) \
  f(fcvt_w_d_rmm) f(fcvt_wu_d_rmm) f(fcvt_l_d_rmm) f(fcvt_lu_d_rmm) \

#define INSTR_RVC(f) \
  f(c_j) f(p_jal) f(c_jr) f(c_jalr) \
  f(c_beqz) f(c_bnez) f(c_mv) f(p_sext_w) \
  f(c_li) f(c_addi) f(c_slli) f(c_srli) f(c_srai) f(c_andi) f(c_addiw) \
  f(c_add) f(c_and) f(c_or) f(c_xor) f(c_sub) f(c_addw) f(c_subw) \
  f(c_addix_sp) f(c_addisp_sp) \
  f(p_li_0) f(p_li_1) f(p_inc) f(p_dec) f(p_mv_src1) f(p_mv_src2) \
  f(p_blez) f(p_bgez) f(p_bltz) f(p_bgtz) \
  f(p_not) f(p_neg) f(p_negw) f(p_seqz) f(p_snez) f(p_sltz) f(p_sgtz) \
  f(p_fmv_s) f(p_fabs_s) f(p_fneg_s) f(p_fmv_d) f(p_fabs_d) f(p_fneg_d) \
  f(p_li_ra) f(p_li_t0) f(p_jalr_ra) f(p_jalr_t0) f(p_jalr_ra_noimm) \
  f(p_jal_next) f(c_j_next) \
  f(c_beqz_tnext) f(c_bnez_tnext) f(c_beqz_ntnext) f(c_bnez_ntnext) \
  f(p_blez_tnext) f(p_bgtz_tnext) f(p_bltz_tnext) f(p_bgez_tnext) \
  f(p_blez_ntnext) f(p_bgtz_ntnext) f(p_bltz_ntnext) f(p_bgez_ntnext) \

#define INSTR_SPECIAL(f) \
  f(inv) f(rt_inv) f(nemu_trap) \


#define INSTR_LIST(f) f(atomic) f(system) \
  INSTR_RVI(f) \
  INSTR_RVM(f) \
  INSTR_RVF(f) \
  INSTR_RVD(f) \
  INSTR_RVC(f) \
  INSTR_SPECIAL(f) \

#else
#define INSTR_LIST(f) f(auipc) f(ld) f(sd) f(inv) f(nemu_trap)
#endif

def_all_EXEC_ID();
