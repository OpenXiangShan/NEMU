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
    // IFNDEF(CONFIG_DIFFTEST_REF_NEMU, difftest_skip_dut(1, 2));
#ifdef CONFIG_SHARE
    if (s->isa.instr.val == 0x73) { // ecall
#else
    if (id == 0) { // ecall
#endif
#ifdef CONFIG_RVH
      rtl_trap(s, s->pc, 8 + cpu.mode + (cpu.mode == MODE_S && cpu.v));
#else
      rtl_trap(s, s->pc, 8 + cpu.mode);
#endif
      rtl_mv(s, jpc, t0);
#ifdef CONFIG_RV_DEBUG
    } else if (id == 1) { // ebreak
      rtl_trap(s, s->pc, 3);
      rtl_mv(s, jpc, t0);
#elif defined(CONFIG_EBREAK_AS_TRAP)
    } else if (id == 1) { // ebreak
      // Please keep the following lines same as in src/isa/riscv64/instr/special.h.
      rtl_hostcall(s, HOSTCALL_EXIT, NULL, &cpu.gpr[10]._64, NULL, 0); // gpr[10] is $a0
      longjmp_context(NEMU_EXEC_END);
#endif
    } else {
      rtl_hostcall(s, HOSTCALL_PRIV, jpc, src1, NULL, id);
    }
    // is_jmp: ecall, ebreak, mret, sret, mnret
    int is_jmp = (id == 0) || (id == 1) || (id == 0x102) || (id == 0x302) ||  MUXDEF(CONFIG_RV_SMRNMI, (id == 0x702), false);
    return is_jmp;
  }

  save_globals(s);
  // IFNDEF(CONFIG_DIFFTEST_REF_NEMU, difftest_skip_dut(1, 3));

  // funct3 != 0: CSRRW, CSRRS, CSRRC, CSRRWI, CSRRSI, CSRRCI
  rtl_li(s, s1, BITS(funct3, 2, 2) ? s->isa.instr.i.rs1 : *src1);
  rtl_li(s, s2, id);
  rtl_hostcall(s, HOSTCALL_CSR, s0, s1, s2, s->isa.instr.val);
  // update dest register
  rtl_mv(s, dest, s0);

  return 0;
}
