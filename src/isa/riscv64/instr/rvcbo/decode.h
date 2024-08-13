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

def_THelper(cbo) {
  int mmu_mode = isa_mmu_state();
  if (mmu_mode == MMU_DIRECT) {
    def_INSTR_TAB("0000000 00100 ????? ??? ????? ????? ??", cbo_zero);
  } else if (mmu_mode == MMU_TRANSLATE) {
    def_INSTR_TAB("0000000 00100 ????? ??? ????? ????? ??", cbo_zero_mmu);
  } else { assert(0); }
  def_INSTR_TAB(  "0000000 00000 ????? ??? ????? ????? ??", cbo_inval);
  def_INSTR_TAB(  "0000000 00010 ????? ??? ????? ????? ??", cbo_flush);
  def_INSTR_TAB(  "0000000 00001 ????? ??? ????? ????? ??", cbo_clean);
  return EXEC_ID_inv;
}
