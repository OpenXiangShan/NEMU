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

#ifdef CONFIG_RVV

#include "cpu/exec.h"
#include "../local-include/vreg.h"
#include "../local-include/csr.h"
#include <stdio.h>
#include "../local-include/intr.h"
#include "../local-include/rtl.h"
#include <setjmp.h>

int get_mode(Decode *s) {
  rtl_lr(s, &(id_src1->val), id_src1->reg, 4);
  int mode = 0;
  if (id_src1->reg == 0 && id_dest->reg != 0) {
    mode = 1;
  }
  else if (id_src1->reg == 0 && id_dest->reg == 0) {
    mode = 2;
  }
  return mode;
}

void set_vtype_vl(Decode *s, int mode) {
  rtlreg_t vl_num = check_vsetvl(id_src2->val, id_src1->val, mode);
  rtlreg_t error = 1ul << 63;
  
  if(vl_num == (uint64_t)-1) {
    vtype->val = error;
  }
  else {
    vtype->val = id_src2->val;
  }
  
  vl->val = vl_num;

  rtl_sr(s, id_dest->reg, &vl_num, 8/*4*/);

  vstart->val = 0;
}

def_EHelper(vsetvl) {

  //vlmul+lg2(VLEN) <= vsew + vl
  // previous decode does not load vals for us
  int mode = get_mode(s);
  set_vtype_vl(s, mode);
  set_mstatus_dirt();
  // print_asm_template3(vsetvl);
}

def_EHelper(vsetvli) {

  //vlmul+lg2(VLEN) <= vsew + vl
  // previous decode does not load vals for us
  int mode = get_mode(s);
  set_vtype_vl(s, mode);
  set_mstatus_dirt();
  // print_asm_template3(vsetvl);
}

def_EHelper(vsetivli) {
  //vlmul+lg2(VLEN) <= vsew + vl
  // previous decode does not load vals for us
  set_vtype_vl(s, 0);
  set_mstatus_dirt();
  // print_asm_template3(vsetvl);
}

#endif // CONFIG_RVV