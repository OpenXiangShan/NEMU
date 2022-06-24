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

def_EHelper(mfhi) {
  rtl_mv(s, ddest, &cpu.hi);
}

def_EHelper(mflo) {
  rtl_mv(s, ddest, &cpu.lo);
}

def_EHelper(mthi) {
  rtl_mv(s, &cpu.hi, dsrc1);
}

def_EHelper(mtlo) {
  rtl_mv(s, &cpu.lo, dsrc1);
}

def_EHelper(mul) {
  rtl_mulu_lo(s, ddest, dsrc1, dsrc2);
}

def_EHelper(mult) {
  rtl_mulu_lo(s, &cpu.lo, dsrc1, dsrc2);
  rtl_muls_hi(s, &cpu.hi, dsrc1, dsrc2);
}

def_EHelper(multu) {
  rtl_mulu_lo(s, &cpu.lo, dsrc1, dsrc2);
  rtl_mulu_hi(s, &cpu.hi, dsrc1, dsrc2);
}

def_EHelper(div) {
  rtl_divs_q(s, &cpu.lo, dsrc1, dsrc2);
  rtl_divs_r(s, &cpu.hi, dsrc1, dsrc2);
}

def_EHelper(divu) {
  rtl_divu_q(s, &cpu.lo, dsrc1, dsrc2);
  rtl_divu_r(s, &cpu.hi, dsrc1, dsrc2);
}
