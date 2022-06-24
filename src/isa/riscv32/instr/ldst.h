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

#ifdef __ICS_EXPORT
def_EHelper(lw) {
  rtl_lms(s, ddest, dsrc1, id_src2->imm, 4);
}

def_EHelper(sw) {
  rtl_sm(s, ddest, dsrc1, id_src2->imm, 4);
}
#else
#define def_ldst_template(name, rtl_instr, width, mmu_mode) \
  def_EHelper(name) { \
    concat(rtl_, rtl_instr) (s, ddest, dsrc1, id_src2->imm, width, mmu_mode); \
  }

#define def_all_ldst(suffix, mmu_mode) \
  def_ldst_template(concat(lw , suffix), lms, 4, mmu_mode) \
  def_ldst_template(concat(lh , suffix), lms, 2, mmu_mode) \
  def_ldst_template(concat(lb , suffix), lms, 1, mmu_mode) \
  def_ldst_template(concat(lhu, suffix), lm , 2, mmu_mode) \
  def_ldst_template(concat(lbu, suffix), lm , 1, mmu_mode) \
  def_ldst_template(concat(sw , suffix), sm , 4, mmu_mode) \
  def_ldst_template(concat(sh , suffix), sm , 2, mmu_mode) \
  def_ldst_template(concat(sb , suffix), sm , 1, mmu_mode)

def_all_ldst(, MMU_DIRECT)
def_all_ldst(_mmu, MMU_TRANSLATE)
#endif
