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

// The following RVC instructions are excluded
// (1) not present in RV64
//       C.LQSP  C.SQSP
//       C.LQ    C.SQ
//       C.JAL
// (2) seem not frequently present during execution
//       C.LDSP  C.LWSP  C.SDSP  C.SWSP
//       C.ADDI4SPN
// (3) only expansion without optimization
//       C.LD    C.LW    C.SD    C.SW
//       C.LUI
// (4) still not considered
//       C.FLDSP C.FLWSP C.FSDSP C.FSWSP
//       C.FLD   C.FLW   C.FSD   C.FSW
// (5) redundant from the aspect of EHelper
//       C.ADDI16SP (the same as C.ADDI)
//       C.NOP      (the same as C.ADDI)

def_EHelper(c_j) {
  rtl_j(s, id_src1->imm);
}

def_EHelper(c_jr) {
#ifdef CONFIG_SHARE
  // See rvi/control.h:26. JALR should set the LSB to 0.
  rtl_andi(s, s0, dsrc1, ~1UL);
  rtl_jr(s, s0);
#else
//  IFDEF(CONFIG_ENGINE_INTERPRETER, rtl_andi(s, s0, s0, ~0x1u));
  IFNDEF(CONFIG_DIFFTEST_REF_NEMU, difftest_skip_dut(1, 2));
  rtl_jr(s, dsrc1);
#endif
}

def_EHelper(c_jalr) {
  rtl_li(s, &cpu.gpr[1]._64, s->snpc);
#ifdef CONFIG_SHARE
  // See rvi/control.h:26. JALR should set the LSB to 0.
  rtl_andi(s, s0, dsrc1, ~1UL);
  rtl_jr(s, s0);
#else
//  IFDEF(CONFIG_ENGINE_INTERPRETER, rtl_andi(s, s0, s0, ~0x1lu));
  IFNDEF(CONFIG_DIFFTEST_REF_NEMU, difftest_skip_dut(1, 2));
  rtl_jr(s, dsrc1);
#endif
}

def_EHelper(c_beqz) {
  rtl_jrelop(s, RELOP_EQ, dsrc1, rz, id_dest->imm);
}

def_EHelper(c_bnez) {
  rtl_jrelop(s, RELOP_NE, dsrc1, rz, id_dest->imm);
}

def_EHelper(c_li) {
  rtl_li(s, ddest, id_src2->imm);
}

def_EHelper(c_addi) {
  rtl_addi(s, ddest, ddest, id_src2->imm);
}

def_EHelper(c_addiw) {
  rtl_addiw(s, ddest, ddest, id_src2->imm);
}

def_EHelper(c_slli) {
  rtl_shli(s, ddest, ddest, id_src2->imm);
}

def_EHelper(c_srli) {
  rtl_shri(s, ddest, ddest, id_src2->imm);
}

def_EHelper(c_srai) {
  rtl_sari(s, ddest, ddest, id_src2->imm);
}

def_EHelper(c_andi) {
  rtl_andi(s, ddest, ddest, id_src2->imm);
}

def_EHelper(c_mv) {
  rtl_mv(s, ddest, dsrc1);
}

def_EHelper(c_add) {
  rtl_add(s, ddest, ddest, dsrc2);
}

def_EHelper(c_and) {
  rtl_and(s, ddest, ddest, dsrc2);
}

def_EHelper(c_or) {
  rtl_or(s, ddest, ddest, dsrc2);
}

def_EHelper(c_xor) {
  rtl_xor(s, ddest, ddest, dsrc2);
}

def_EHelper(c_sub) {
  rtl_sub(s, ddest, ddest, dsrc2);
}

def_EHelper(c_addw) {
  rtl_addw(s, ddest, ddest, dsrc2);
}

def_EHelper(c_subw) {
  rtl_subw(s, ddest, ddest, dsrc2);
}
