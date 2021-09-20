#include <cpu/decode.h>
#include "../local-include/rtl.h"

#ifndef __ICS_EXPORT

#define INSTR_RVI(f) \
  f(lui) f(auipc) f(jal) \
  f(ld) f(lw) f(lh) f(lb) f(lwu) f(lhu) f(lbu) f(sd) f(sw) f(sh) f(sb) \
  f(ld_mmu) f(lw_mmu) f(lh_mmu) f(lb_mmu) f(lwu_mmu) f(lhu_mmu) f(lbu_mmu) \
  f(sd_mmu) f(sw_mmu) f(sh_mmu) f(sb_mmu) \
  f(add) f(sll) f(srl) f(slt) f(sltu) f(xor) f(or) f(sub) f(sra) f(and) \
  f(addi) f(slli) f(srli) f(slti) f(sltui) f(xori) f(ori) f(srai) f(andi) \
  f(jalr) f(beq) f(bne) f(blt) f(bge) f(bltu) f(bgeu) \
  f(addw) f(sllw) f(srlw) f(subw) f(sraw) \
  f(addiw) f(slliw) f(srliw) f(sraiw) \
  f(fence_i) f(fence) \
  f(p_ret) \

#define INSTR_RVM(f) \
  f(mul) f(mulh) f(mulhu) f(mulhsu) f(div) f(divu) f(rem) f(remu) \
  f(mulw) f(divw) f(divuw) f(remw) f(remuw) \

#define INSTR_RVF(f) \
  f(flw) f(fsw) f(flw_mmu) f(fsw_mmu) \
  f(fadds) f(fsubs) f(fmuls) f(fdivs) f(fmins) f(fmaxs) \
  f(fsqrts) f(fles) f(flts) f(feqs) \
  f(fmadds) f(fmsubs) f(fnmsubs) f(fnmadds) \
  f(fsgnjs) f(fsgnjns) f(fsgnjxs) \
  f(fcvt_s_w) f(fcvt_s_wu) f(fcvt_s_l) f(fcvt_s_lu) \
  f(fcvt_w_s) f(fcvt_wu_s) f(fcvt_l_s) f(fcvt_lu_s) \
  f(fmv_x_w) f(fmv_w_x) \

#define INSTR_RVD(f) \
  f(fld) f(fsd) f(fld_mmu) f(fsd_mmu) \
  f(faddd) f(fsubd) f(fmuld) f(fdivd) f(fmind) f(fmaxd) \
  f(fsqrtd) f(fled) f(fltd) f(feqd) \
  f(fmaddd) f(fmsubd) f(fnmsubd) f(fnmaddd) \
  f(fsgnjd) f(fsgnjnd) f(fsgnjxd) \
  f(fcvt_d_w) f(fcvt_d_wu) f(fcvt_d_l) f(fcvt_d_lu) \
  f(fcvt_w_d) f(fcvt_wu_d) f(fcvt_l_d) f(fcvt_lu_d) \
  f(fmv_x_d) f(fmv_d_x) \
  f(fcvt_d_s) f(fcvt_s_d)

#define INSTR_RVC(f) \
  f(c_j) f(p_jal) f(c_jr) f(c_jalr) \
  f(c_beqz) f(c_bnez) f(c_mv) f(p_sext_w) \
  f(c_li) f(c_addi) f(c_slli) f(c_srli) f(c_srai) f(c_andi) f(c_addiw) \
  f(c_add) f(c_and) f(c_or) f(c_xor) f(c_sub) f(c_addw) f(c_subw) \
  f(p_li_0) f(p_li_1) \
  f(p_blez) f(p_bgez) f(p_bltz) f(p_bgtz) \
  f(p_inc) f(p_dec) \

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
