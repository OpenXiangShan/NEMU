/***************************************************************************************
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

#include <common.h>

#ifdef CONFIG_RVMATRIX

#include "cpu/exec.h"
#include "../local-include/csr.h"
#include <stdio.h>
#include "../local-include/intr.h"
#include "../local-include/rtl.h"
#include "../local-include/reg.h"

def_EHelper(msettype) {
  // read mtype value from rs1
  s->src1.val = reg_l(s->src1.reg);
  // write mtype value to mtype_csr_reg and rd
  mtype->val = s->src1.val;
  reg_l(s->dest.reg) = s->src1.val;
}

def_EHelper(msettypei) {
  mtype->val = s->src2.imm;
  reg_l(s->dest.reg) = s->src2.imm;
}

def_EHelper(msettilem) {
  s->src1.val = reg_l(s->src1.reg);
  mtype->val = s->src1.val;
  reg_l(s->dest.reg) = s->src1.val;
}

def_EHelper(msettilemi) {
  mtype->val = s->src2.imm;
  reg_l(s->dest.reg) = s->src2.imm;
}

def_EHelper(msettilek) {
  s->src1.val = reg_l(s->src1.reg);
  mtype->val = s->src1.val;
  reg_l(s->dest.reg) = s->src1.val;
}

def_EHelper(msettileki) {
  mtype->val = s->src2.imm;
  reg_l(s->dest.reg) = s->src2.imm;
}

def_EHelper(msettilen) {
  s->src1.val = reg_l(s->src1.reg);
  mtype->val = s->src1.val;
  reg_l(s->dest.reg) = s->src1.val;
}

def_EHelper(msettileni) {
  mtype->val = s->src2.imm;
  reg_l(s->dest.reg) = s->src2.imm;
}

#endif // CONFIG_RVMATRIX