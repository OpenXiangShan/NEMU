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
#include <generated/autoconf.h>
#ifdef CONFIG_BR_LOG
extern uint64_t br_count;
#endif // CONFIG_BR_LOG
def_EHelper(jal) {
  rtl_li(s, ddest, id_src2->imm);
  // printf("%lx,%lx.%d,%d,%lx\n", br_count, cpu.pc, 1, 1, id_src1->imm);
#ifdef CONFIG_BR_LOG
  br_log[br_count].pc = s->pc; // cpu.pc - 4;
  br_log[br_count].target = id_src1->imm;
  br_log[br_count].taken = 1;
  br_log[br_count].type = 1;
  br_count++;
#endif // CONFIG_BR_LOG
  rtl_j(s, id_src1->imm);
}

def_EHelper(jalr) {
  // Described at 2.5 Control Transter Instructions
  // The target address is obtained by adding the sign-extended 12-bit I-immediate to the register rs1
  rtl_addi(s, s0, dsrc1, id_src2->imm);
  // then setting the least-significant bit of the result to zero.
  rtl_andi(s, s0, s0, ~1UL);
//  IFDEF(CONFIG_ENGINE_INTERPRETER, rtl_andi(s, s0, s0, ~0x1lu));
#ifdef CONFIG_GUIDED_EXEC
  if(cpu.guided_exec && cpu.execution_guide.force_set_jump_target) {
    rtl_li(s, ddest, cpu.execution_guide.jump_target);
  } else {
    rtl_li(s, ddest, s->snpc);
  }
#else
  rtl_li(s, ddest, s->snpc);
#endif
  IFNDEF(CONFIG_DIFFTEST_REF_NEMU, difftest_skip_dut(1, 3));
  rtl_jr(s, s0);
  //printf("%lx,%lx,%d,%d,%lx\n", br_count, cpu.pc, 1, 1, *s0);
}

def_EHelper(beq) {
  rtl_jrelop(s, RELOP_EQ, dsrc1, dsrc2, id_dest->imm);
}

def_EHelper(bne) {
  rtl_jrelop(s, RELOP_NE, dsrc1, dsrc2, id_dest->imm);
}

def_EHelper(blt) {
  rtl_jrelop(s, RELOP_LT, dsrc1, dsrc2, id_dest->imm);
}

def_EHelper(bge) {
  rtl_jrelop(s, RELOP_GE, dsrc1, dsrc2, id_dest->imm);
}

def_EHelper(bltu) {
  rtl_jrelop(s, RELOP_LTU, dsrc1, dsrc2, id_dest->imm);
}

def_EHelper(bgeu) {
  rtl_jrelop(s, RELOP_GEU, dsrc1, dsrc2, id_dest->imm);
}
