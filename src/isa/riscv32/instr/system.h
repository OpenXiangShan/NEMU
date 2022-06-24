/***************************************************************************************
* Copyright (c) 2014-2021 Zihao Yu, Nanjing University
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

#ifndef __ICS_EXPORT

#define csr_difftest()  IFNDEF(CONFIG_DIFFTEST_REF_NEMU, difftest_skip_dut(1, 3))
#define priv_difftest() IFNDEF(CONFIG_DIFFTEST_REF_NEMU, difftest_skip_dut(1, 2))

def_EHelper(csrrw) {
  csr_difftest();
  rtl_hostcall(s, HOSTCALL_CSR, ddest, dsrc1, id_src2->imm);
  rtl_priv_next(s);
}

def_EHelper(csrrs) {
  csr_difftest();
  rtl_hostcall(s, HOSTCALL_CSR, s0, NULL, id_src2->imm);
  rtl_or(s, s1, s0, dsrc1);
  rtl_mv(s, ddest, s0);
  rtl_hostcall(s, HOSTCALL_CSR, NULL, s1, id_src2->imm);
  rtl_priv_next(s);
}

def_EHelper(ecall) {
  priv_difftest();
  rtl_trap(s, s->pc, 9);
  rtl_priv_jr(s, t0);
}

def_EHelper(sret) {
  priv_difftest();
  rtl_hostcall(s, HOSTCALL_PRIV, s0, NULL, 0x102);
  rtl_priv_jr(s, s0);
}

def_EHelper(sfence_vma) {
  priv_difftest();
  rtl_hostcall(s, HOSTCALL_PRIV, NULL, NULL, 0x120);
  rtl_priv_next(s);
}
#endif
