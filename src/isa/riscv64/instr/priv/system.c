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
      longjmp_exec(NEMU_EXEC_END);
      printf("hahaha\n");
#endif
    } else {
      rtl_hostcall(s, HOSTCALL_PRIV, jpc, src1, NULL, id);
    }
    // is_jmp: ecall, ebreak, mret, sret
    int is_jmp = (id == 0) || (id == 1) || (id == 0x102) || (id == 0x302);
    return is_jmp;
  }

  save_globals(s);
  // IFNDEF(CONFIG_DIFFTEST_REF_NEMU, difftest_skip_dut(1, 3));

  // funct3 != 0: CSRRW, CSRRS, CSRRC, CSRRWI, CSRRSI, CSRRCI

  // read the CSR
  rtl_hostcall(s, HOSTCALL_CSR, s0, NULL, NULL, id);

  // write the CSR
  // According to RISC-V spec, CSRRS[I], CSRRC[I] will not write to the CSRs.
  // (1) For both CSRRS and CSRRC, if rs1=x0, then the instruction will not
  // write to the CSR at all, and so shall not cause any of the side effects
  // that might otherwise occur on a CSR write, such as raising illegal
  // instruction exceptions on accesses to read-only CSRs.
  // (2) For CSRRSI and CSRRCI, if the uimm[4:0] field is zero, then these
  // instructions will not write to the CSR, and shall not cause any of the
  // side effects that might otherwise occur on a CSR write.
  uint32_t is_csrrs_or_csrrc = funct3 & 0x2;
  if (!is_csrrs_or_csrrc || s->isa.instr.i.rs1 != 0) {
    int imm = funct3 & 0x4;
    int op  = funct3 & 0x3;
    if (imm) rtl_li(s, s1, s->isa.instr.i.rs1);
    else rtl_mv(s, s1, src1);
    switch (op) {
      case 2: rtl_or(s, s1, s0, s1); break;
      case 3: rtl_not(s, s1, s1); rtl_and(s, s1, s0, s1); break;
    }
    rtl_hostcall(s, HOSTCALL_CSR, NULL, s1, NULL, id);
  }

  // update dest register
  rtl_mv(s, dest, s0);

  return 0;
}
