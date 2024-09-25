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
#include "../local-include/rtl.h"
#include "vcommon.h"
#include "vcompute_impl.h"

def_EHelper(vsetvl) {

  require_vector(false);
  rtl_lr(s, &(s->src1.val), s->src1.reg, 4);
  rtl_lr(s, &(s->src2.val), s->src2.reg, 4);
  int mode = get_mode(s);
  set_vtype_vl(s, mode);
  vp_set_dirty();
}

def_EHelper(vsetvli) {

  require_vector(false);
  rtl_lr(s, &(s->src1.val), s->src1.reg, 4);
  rtl_li(s, &(s->src2.val), s->isa.instr.v_opsimm.v_zimm);
  int mode = get_mode(s);
  set_vtype_vl(s, mode);
  vp_set_dirty();
}

def_EHelper(vsetivli) {

  require_vector(false);
  rtl_li(s, &(s->src1.val), s->isa.instr.v_vseti.v_zimm5);
  rtl_li(s, &(s->src2.val), s->isa.instr.v_vseti.v_zimm);
  set_vtype_vl(s, 0);
  vp_set_dirty();
}

#endif // CONFIG_RVV