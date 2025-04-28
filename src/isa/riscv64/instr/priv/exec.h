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

def_EHelper(ecall) {
#ifdef CONFIG_RVH
  rtl_trap(s, s->pc, 8 + cpu.mode + (cpu.mode == MODE_S && cpu.v));
#else
  rtl_trap(s, s->pc, 8 + cpu.mode);
#endif
  rtl_mv(s, s0, t0);
  rtl_priv_jr(s, s0);
}

def_EHelper(ebreak) {
#ifdef CONFIG_EBREAK_AS_TRAP
  // Please keep the following lines same as in src/isa/riscv64/instr/special.h.
  rtl_hostcall(s, HOSTCALL_EXIT, NULL, &cpu.gpr[10]._64, NULL, 0); // gpr[10] is $a0
  longjmp_context(NEMU_EXEC_END);
#else // CONFIG_EBREAK_AS_TRAP
  rtl_trap(s, s->pc, 3);
  rtl_mv(s, s0, t0);
  rtl_priv_jr(s, s0);
#endif // CONFIG_EBREAK_AS_TRAP
}

def_EHelper(sret) {
  *s0 = riscv64_priv_sret();
  rtl_priv_jr(s, s0);
}

def_EHelper(mret) {
  *s0 = riscv64_priv_mret();
  rtl_priv_jr(s, s0);
}

#ifdef CONFIG_RV_SMRNMI
def_EHelper(mnret) {
  *s0 = riscv64_priv_mnret();
  rtl_priv_jr(s, s0);
}
#endif // CONFIG_RV_SMRNMI

def_EHelper(wfi) {
  riscv64_priv_wfi();
  rtl_priv_next(s);
}

#ifdef CONFIG_RV_ZAWRS
def_EHelper(wrs_nto) {
  riscv64_priv_wrs_nto();
  rtl_priv_next(s);
}

def_EHelper(wrs_sto) {
  rtl_priv_next(s);
}
#endif // CONFIG_RV_ZAWRS

def_EHelper(sfence_vma) {
  riscv64_priv_sfence_vma(*dsrc1, *ddest);
  rtl_priv_next(s);
}

#ifdef CONFIG_RVH
def_EHelper(hfence_vvma) {
  riscv64_priv_hfence_vvma(*dsrc1, *ddest);
  rtl_priv_next(s);
}

def_EHelper(hfence_gvma) {
  riscv64_priv_hfence_gvma(*dsrc1, *ddest);
  rtl_priv_next(s);
}
#endif // CONFIG_RVH

#ifdef CONFIG_RV_SVINVAL
def_EHelper(sfence_w_inval) {
  riscv64_priv_sfence_w_inval_ir();
  rtl_priv_next(s);
}

def_EHelper(sfence_inval_ir) {
  riscv64_priv_sfence_w_inval_ir();
  rtl_priv_next(s);
}

def_EHelper(sinval_vma) {
  riscv64_priv_sfence_vma(*dsrc1, *ddest);
  rtl_priv_next(s);
}

#ifdef CONFIG_RVH
def_EHelper(hinval_vvma) {
  riscv64_priv_hfence_vvma(*dsrc1, *ddest);
  rtl_priv_next(s);
}

def_EHelper(hinval_gvma) {
  riscv64_priv_hfence_gvma(*dsrc1, *ddest);
  rtl_priv_next(s);
}
#endif // CONFIG_RVH
#endif // CONFIG_RV_SVINVAL

#ifdef CONFIG_RV_ZIMOP
// EHelpers for Zimop Instructions

// mop.r.n and mop.rr.n write 0 to x[rd] without redefinition
// It's no need to distinguish n.

def_EHelper(mop_r_n) {
  rtl_li(s, ddest, 0);
}

def_EHelper(mop_rr_n) {
  rtl_li(s, ddest, 0);
}

#endif // CONFIG_RV_ZIMOP

#ifdef CONFIG_RVH
// EHelpers for Hypervisor Virtual-Machine Load and Store Instructions

#define def_hld_template(name, len, is_signed, is_hlvx) \
def_EHelper(name) { \
  riscv64_priv_hload(s, ddest, dsrc1, len, is_signed, is_hlvx); \
}

def_hld_template(hlv_b,   1, true,  false)
def_hld_template(hlv_bu,  1, false, false)
def_hld_template(hlv_h,   2, true,  false)
def_hld_template(hlv_hu,  2, false, false)
def_hld_template(hlv_w,   4, true,  false)
def_hld_template(hlv_wu,  4, false, false)
def_hld_template(hlv_d,   8, true,  false)

def_hld_template(hlvx_hu, 2, false, true)
def_hld_template(hlvx_wu, 4, false, true)

#define def_hst_template(name, len) \
def_EHelper(name) { \
  riscv64_priv_hstore(s, ddest, dsrc1, len); \
}

def_hst_template(hsv_b, 1)
def_hst_template(hsv_h, 2)
def_hst_template(hsv_w, 4)
def_hst_template(hsv_d, 8)

#endif // CONFIG_RVH

def_EHelper(csrrw) {
  save_globals(s);
  riscv64_priv_csrrw(ddest, *dsrc1, id_src2->imm, s->isa.instr.i.rd);
  rtl_priv_next(s);
}

def_EHelper(csrrs) {
  save_globals(s);
  riscv64_priv_csrrs(ddest, *dsrc1, id_src2->imm, s->isa.instr.i.rs1);
  rtl_priv_next(s);
}

def_EHelper(csrrc) {
  save_globals(s);
  riscv64_priv_csrrc(ddest, *dsrc1, id_src2->imm, s->isa.instr.i.rs1);
  rtl_priv_next(s);
}

def_EHelper(csrrwi) {
  save_globals(s);
  riscv64_priv_csrrw(ddest, id_src1->imm, id_src2->imm, s->isa.instr.i.rd);
  rtl_priv_next(s);
}

def_EHelper(csrrsi) {
  save_globals(s);
  riscv64_priv_csrrs(ddest, id_src1->imm, id_src2->imm, s->isa.instr.i.rs1);
  rtl_priv_next(s);
}

def_EHelper(csrrci) {
  save_globals(s);
  riscv64_priv_csrrc(ddest, id_src1->imm, id_src2->imm, s->isa.instr.i.rs1);
  rtl_priv_next(s);
}
