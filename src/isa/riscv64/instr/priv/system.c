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

#include <rtl/rtl.h>
#include <cpu/difftest.h>
#include <cpu/cpu.h>

__attribute__((cold))
int rtl_sys_slow_path(Decode *s, rtlreg_t *dest, const rtlreg_t *src1, uint32_t id, rtlreg_t *jpc) {
  uint32_t funct3 = s->isa.instr.i.funct3;
  if (funct3 == 0) {
    // priv
    IFNDEF(CONFIG_DIFFTEST_REF_NEMU, difftest_skip_dut(1, 2));
#ifdef CONFIG_SHARE
    if (s->isa.instr.val == 0x73) { // ecall
#else
    if (id == 0) { // ecall
#endif
      rtl_trap(s, s->pc, 8 + cpu.mode);
      rtl_mv(s, jpc, t0);
#ifdef CONFIG_RV_DEBUG
    } else if (id == 1) { // ebreak
      rtl_trap(s, s->pc, 3);
      rtl_mv(s, jpc, t0);
#endif
    } else {
      rtl_hostcall(s, HOSTCALL_PRIV, jpc, src1, NULL, id);
    }
    // is_jmp: ecall, ebreak, mret, sret
    int is_jmp = (id == 0) || (id == 1) || (id == 0x102) || (id == 0x302);
    return is_jmp;
  }

  save_globals(s);
  IFNDEF(CONFIG_DIFFTEST_REF_NEMU, difftest_skip_dut(1, 3));

  rtl_hostcall(s, HOSTCALL_CSR, s0, NULL, NULL, id);
  int imm = funct3 & 0x4;
  int op  = funct3 & 0x3;
  if (imm) rtl_li(s, s1, s->isa.instr.i.rs1);
  else rtl_mv(s, s1, src1);
  switch (op) {
    case 2: rtl_or(s, s1, s0, s1); break;
    case 3: rtl_not(s, s1, s1); rtl_and(s, s1, s0, s1); break;
  }
  rtl_hostcall(s, HOSTCALL_CSR, NULL, s1, NULL, id);
  rtl_mv(s, dest, s0);
  return 0;
}
