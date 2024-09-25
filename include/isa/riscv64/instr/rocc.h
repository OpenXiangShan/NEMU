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

rtlreg_t rocc_sdi(uint32_t funct7, uint32_t funct3, rtlreg_t rs1, rtlreg_t rs2);

static inline def_EHelper(rocc3) {
  *ddest = rocc_sdi(s->isa.instr.r.funct7, s->isa.instr.r.funct3, *dsrc1, *dsrc2);
  print_asm_template3(rocc3);
}
