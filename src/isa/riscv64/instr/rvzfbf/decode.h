/***************************************************************************************
* Copyright (c) 2020-2026 Institute of Computing Technology, Chinese Academy of Sciences
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

#ifdef CONFIG_RV_ZFBF_MIN
def_THelper(op_zfbf) {
  def_INSTR_IDTAB("01000 10 01000 ????? ??? ????? ????? ??", fr, fcvt_bf16_s);
  def_INSTR_IDTAB("01000 00 00110 ????? ??? ????? ????? ??", fr, fcvt_s_bf16);
  return EXEC_ID_inv;
}
#endif // CONFIG_RV_ZFBF_MIN
