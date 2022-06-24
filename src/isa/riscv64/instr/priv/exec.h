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

#ifdef CONFIG_DEBUG
#ifdef CONFIG_RV_SVINVAL
#define SYS_INSTR_LIST(f) \
  f(csrrw)  f(csrrs)  f(csrrc) f(csrrwi) f(csrrsi) f(csrrci) \
  f(ecall)  f(mret)   f(sret)  f(sfence_vma) f(wfi) \
  f(sfence_w_inval) f(sfence_inval_ir) f(sinval_vma)
#else
#define SYS_INSTR_LIST(f) \
  f(csrrw)  f(csrrs)  f(csrrc) f(csrrwi) f(csrrsi) f(csrrci) \
  f(ecall)  f(mret)   f(sret)  f(sfence_vma) f(wfi)
#endif

MAP(SYS_INSTR_LIST, def_SYS_EHelper)
#else
def_SYS_EHelper(system)
#endif
