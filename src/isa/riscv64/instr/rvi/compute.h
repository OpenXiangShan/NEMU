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

def_EHelper(add) {
  rtl_add(s, ddest, dsrc1, dsrc2);
}

def_EHelper(sub) {
  rtl_sub(s, ddest, dsrc1, dsrc2);
}

def_EHelper(sll) {
  rtl_shl(s, ddest, dsrc1, dsrc2);
}

def_EHelper(sra) {
  rtl_sar(s, ddest, dsrc1, dsrc2);
}

def_EHelper(srl) {
  rtl_shr(s, ddest, dsrc1, dsrc2);
}

def_EHelper(slt) {
  rtl_setrelop(s, RELOP_LT, ddest, dsrc1, dsrc2);
}

def_EHelper(sltu) {
  rtl_setrelop(s, RELOP_LTU, ddest, dsrc1, dsrc2);
}

def_EHelper(xor) {
  rtl_xor(s, ddest, dsrc1, dsrc2);
}

def_EHelper(or) {
  rtl_or(s, ddest, dsrc1, dsrc2);
}

def_EHelper(and) {
  rtl_and(s, ddest, dsrc1, dsrc2);
}

def_EHelper(addi) {
  rtl_addi(s, ddest, dsrc1, id_src2->imm);
}

def_EHelper(slli) {
  rtl_shli(s, ddest, dsrc1, id_src2->imm);
}

def_EHelper(srai) {
  rtl_sari(s, ddest, dsrc1, id_src2->imm);
}

def_EHelper(srli) {
  rtl_shri(s, ddest, dsrc1, id_src2->imm);
}

def_EHelper(slti) {
  rtl_setrelopi(s, RELOP_LT, ddest, dsrc1, id_src2->imm);
}

def_EHelper(sltui) {
  rtl_setrelopi(s, RELOP_LTU, ddest, dsrc1, id_src2->imm);
}

def_EHelper(xori) {
  rtl_xori(s, ddest, dsrc1, id_src2->imm);
}

def_EHelper(ori) {
  rtl_ori(s, ddest, dsrc1, id_src2->imm);
}

def_EHelper(andi) {
  rtl_andi(s, ddest, dsrc1, id_src2->imm);
}

def_EHelper(auipc) {
#ifdef CONFIG_RV_ZICFILP
  if (unlikely(cpu.elp == 1)) {
    uint32_t rd = (s->isa.instr.val >> 7) & 0x1f;
    if (rd != 0) {
      longjmp_exception(EX_SWC);
    }
    if ((s->pc & 0x3) != 0) {
      longjmp_exception(EX_SWC);
    }
    uint32_t lpl = (s->isa.instr.val >> 12) & 0xFFFFF;
    uint32_t x7_lpl = (reg_l(7) >> 12) & 0xFFFFF;

    printf("[ZICFILP-DEBUG] Checking lpad at pc=0x%lx: instr=0x%08x, expected_lpl(x7)=0x%x, actual_lpl=0x%x\n", s->pc, s->isa.instr.val, x7_lpl, lpl);

    if (lpl != x7_lpl && lpl != 0) {
      longjmp_exception(EX_SWC);
    }
    cpu.elp = 0;
  }
#endif
  rtl_li(s, ddest, id_src1->imm);
}

def_EHelper(lui) {
  rtl_li(s, ddest, id_src1->imm);
}

def_EHelper(addw) {
  rtl_addw(s, ddest, dsrc1, dsrc2);
}

def_EHelper(subw) {
  rtl_subw(s, ddest, dsrc1, dsrc2);
}

def_EHelper(sllw) {
  rtl_shlw(s, ddest, dsrc1, dsrc2);
}

def_EHelper(srlw) {
  rtl_shrw(s, ddest, dsrc1, dsrc2);
}

def_EHelper(sraw) {
  rtl_sarw(s, ddest, dsrc1, dsrc2);
}

def_EHelper(addiw) {
  rtl_addiw(s, ddest, dsrc1, id_src2->imm);
}

def_EHelper(slliw) {
  rtl_shliw(s, ddest, dsrc1, id_src2->imm);
}

def_EHelper(srliw) {
  rtl_shriw(s, ddest, dsrc1, id_src2->imm);
}

def_EHelper(sraiw) {
  rtl_sariw(s, ddest, dsrc1, id_src2->imm);
}
