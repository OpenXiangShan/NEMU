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

def_EHelper(p_sext_w) {
  rtl_addiw(s, ddest, dsrc1, 0);
}

def_EHelper(p_blez) {
  rtl_jrelop(s, RELOP_GE, rz, dsrc2, id_dest->imm);
}

def_EHelper(p_bgtz) {
  rtl_jrelop(s, RELOP_LT, rz, dsrc2, id_dest->imm);
}

def_EHelper(p_bltz) {
  rtl_jrelop(s, RELOP_LT, dsrc1, rz, id_dest->imm);
}

def_EHelper(p_bgez) {
  rtl_jrelop(s, RELOP_GE, dsrc1, rz, id_dest->imm);
}

def_EHelper(p_jal) {
  rtl_li(s, &cpu.gpr[1]._64, id_src2->imm);
  rtl_j(s, id_src1->imm);
}

def_EHelper(p_ret) {
#ifdef CONFIG_SHARE
  // See rvi/control.h:26. JALR should set the LSB to 0.
  rtl_andi(s, s0, &cpu.gpr[1]._64, ~1UL);
  rtl_jr(s, s0);
#else
//  IFDEF(CONFIG_ENGINE_INTERPRETER, rtl_andi(s, s0, s0, ~0x1u));
  IFNDEF(CONFIG_DIFFTEST_REF_NEMU, difftest_skip_dut(1, 2));
  rtl_jr(s, &cpu.gpr[1]._64);
#endif // CONFIG_SHARE
}

// non-standard pseudo instructions

def_EHelper(p_li_0) {
  rtl_li(s, ddest, 0);
}

def_EHelper(p_li_1) {
  rtl_li(s, ddest, 1);
}

def_EHelper(p_inc) {
  rtl_addi(s, ddest, ddest, 1);
}

def_EHelper(p_dec) {
  rtl_subi(s, ddest, ddest, 1);
}
