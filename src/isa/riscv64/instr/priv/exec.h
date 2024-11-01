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

#define def_SYS_EHelper(name) \
def_EHelper(name) { \
  extern int rtl_sys_slow_path(Decode *s, rtlreg_t *dest, const rtlreg_t *src1, uint32_t id, rtlreg_t *jpc); \
  int jmp = rtl_sys_slow_path(s, ddest, dsrc1, id_src2->imm, s0); \
  if (jmp) rtl_priv_jr(s, s0); \
  else rtl_priv_next(s); \
}

#if defined(CONFIG_DEBUG) || defined(CONFIG_SHARE)

#ifdef CONFIG_RVH
#define def_hld_template(name) \
  def_EHelper(name){ \
    extern int hload(Decode *s, rtlreg_t *dest, const rtlreg_t * src1, uint32_t id); \
    hload(s, ddest, dsrc1, id_src2->imm); \
  }
#define def_hst_template(name) \
def_EHelper(name){ \
  extern int hstore(Decode *s, rtlreg_t *dest, const rtlreg_t * src1, const rtlreg_t * src2); \
  hstore(s, ddest, dsrc1, dsrc2); \
}
#define RVH_LD_INST_LIST(f) f(hlv_b) f(hlv_bu) f(hlv_h) f(hlv_hu) f(hlvx_hu) f(hlv_w) f(hlvx_wu) f(hlv_wu) f(hlv_d)
#define RVH_ST_INST_LIST(f)  f(hsv_b) f(hsv_h) f(hsv_w) f(hsv_d)
#ifdef CONFIG_RV_SVINVAL
  #define RVH_SYS_INST_LIST(f) f(hfence_vvma) f(hfence_gvma) f(hinval_vvma) f(hinval_gvma)
#else
  #define RVH_SYS_INST_LIST(f) f(hfence_vvma) f(hfence_gvma)
#endif // CONFIG_RV_SVINVAL
MAP(RVH_SYS_INST_LIST, def_SYS_EHelper)
MAP(RVH_LD_INST_LIST, def_hld_template)
MAP(RVH_ST_INST_LIST, def_hst_template)
#endif // CONFIG_RVH

#ifdef CONFIG_RV_SMRNMI
#define SYS_MNINSTR_LIST(f) \
  f(mnret)
#else
#define SYS_MNINSTR_LIST(f)
#endif

#ifdef CONFIG_RV_SVINVAL
#define SYS_INSTR_LIST(f) \
  f(csrrw)  f(csrrs)  f(csrrc) f(csrrwi) f(csrrsi) f(csrrci) \
  f(ecall)  f(ebreak) f(mret)  f(sret)  f(sfence_vma) f(wfi) \
  SYS_MNINSTR_LIST(f)   \
  f(sfence_w_inval) f(sfence_inval_ir) f(sinval_vma)
#else
#define SYS_INSTR_LIST(f) \
  f(csrrw)  f(csrrs)  f(csrrc) f(csrrwi) f(csrrsi) f(csrrci) \
  SYS_MNINSTR_LIST(f)       \
  f(ecall)  f(ebreak) f(mret)  f(sret)  f(sfence_vma) f(wfi)
#endif

MAP(SYS_INSTR_LIST, def_SYS_EHelper)
#else // !(defined(CONFIG_DEBUG) || defined(CONFIG_SHARE))
#ifdef CONFIG_RVH
def_EHelper(hload){
  extern int hload(Decode *s, rtlreg_t *dest, const rtlreg_t * src1, uint32_t id);
  hload(s, ddest, dsrc1, id_src2->imm);
}

def_EHelper(hstore){
  extern int hstore(Decode *s, rtlreg_t *dest, const rtlreg_t * src1, const rtlreg_t * src2);
  hstore(s, ddest, dsrc1, dsrc2);
}

def_SYS_EHelper(priv)
#else
def_SYS_EHelper(system)
#endif

#endif // defined(CONFIG_DEBUG) || defined(CONFIG_SHARE)

// The instructions in ZIMOP extension are very simple, so always use fast path.
#ifdef CONFIG_RV_ZIMOP

def_EHelper(mop_r_0 ) { rtl_i_mop(s, ddest); }
def_EHelper(mop_r_1 ) { rtl_i_mop(s, ddest); }
def_EHelper(mop_r_2 ) { rtl_i_mop(s, ddest); }
def_EHelper(mop_r_3 ) { rtl_i_mop(s, ddest); }
def_EHelper(mop_r_4 ) { rtl_i_mop(s, ddest); }
def_EHelper(mop_r_5 ) { rtl_i_mop(s, ddest); }
def_EHelper(mop_r_6 ) { rtl_i_mop(s, ddest); }
def_EHelper(mop_r_7 ) { rtl_i_mop(s, ddest); }
def_EHelper(mop_r_8 ) { rtl_i_mop(s, ddest); }
def_EHelper(mop_r_9 ) { rtl_i_mop(s, ddest); }
def_EHelper(mop_r_10) { rtl_i_mop(s, ddest); }
def_EHelper(mop_r_11) { rtl_i_mop(s, ddest); }
def_EHelper(mop_r_12) { rtl_i_mop(s, ddest); }
def_EHelper(mop_r_13) { rtl_i_mop(s, ddest); }
def_EHelper(mop_r_14) { rtl_i_mop(s, ddest); }
def_EHelper(mop_r_15) { rtl_i_mop(s, ddest); }
def_EHelper(mop_r_16) { rtl_i_mop(s, ddest); }
def_EHelper(mop_r_17) { rtl_i_mop(s, ddest); }
def_EHelper(mop_r_18) { rtl_i_mop(s, ddest); }
def_EHelper(mop_r_19) { rtl_i_mop(s, ddest); }
def_EHelper(mop_r_20) { rtl_i_mop(s, ddest); }
def_EHelper(mop_r_21) { rtl_i_mop(s, ddest); }
def_EHelper(mop_r_22) { rtl_i_mop(s, ddest); }
def_EHelper(mop_r_23) { rtl_i_mop(s, ddest); }
def_EHelper(mop_r_24) { rtl_i_mop(s, ddest); }
def_EHelper(mop_r_25) { rtl_i_mop(s, ddest); }
def_EHelper(mop_r_26) { rtl_i_mop(s, ddest); }
def_EHelper(mop_r_27) { rtl_i_mop(s, ddest); }
def_EHelper(mop_r_28) { rtl_i_mop(s, ddest); }
def_EHelper(mop_r_29) { rtl_i_mop(s, ddest); }
def_EHelper(mop_r_30) { rtl_i_mop(s, ddest); }
def_EHelper(mop_r_31) { rtl_i_mop(s, ddest); }

def_EHelper(mop_rr_0) { rtl_i_mop(s, ddest); }
def_EHelper(mop_rr_1) { rtl_i_mop(s, ddest); }
def_EHelper(mop_rr_2) { rtl_i_mop(s, ddest); }
def_EHelper(mop_rr_3) { rtl_i_mop(s, ddest); }
def_EHelper(mop_rr_4) { rtl_i_mop(s, ddest); }
def_EHelper(mop_rr_5) { rtl_i_mop(s, ddest); }
def_EHelper(mop_rr_6) { rtl_i_mop(s, ddest); }
def_EHelper(mop_rr_7) { rtl_i_mop(s, ddest); }

#endif // CONFIG_RV_ZIMOP

