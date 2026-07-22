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

#ifdef CONFIG_RV_AME

#include "cpu/exec.h"
#include "ext/amu_ctrl_queue_wrapper.h"
#include <ext/cutest.h>
#ifdef CONFIG_SHARE_REF
#include "ext/msync_queue_wrapper.h"
#endif // CONFIG_SHARE_REF
#include "ext/mstore_queue_wrapper.h"
#include "mcommon.h"
#include "../local-include/csr.h"
#include "../local-include/intr.h"
#include "../local-include/rtl.h"
#include "../local-include/reg.h"
#include <stdio.h>
#include "mcompute_impl.h"

def_EHelper(minit) {
  mstatus->ms = 1;
}

def_EHelper(msettilem) {
  require_matrix();
  mtilem->val = reg_l(s->src1.reg);
  mp_set_dirty();
}

def_EHelper(msettilemi) {
  require_matrix();
  mtilem->val = s->src2.imm;
  mp_set_dirty();
}

def_EHelper(msettilek) {
  require_matrix();
  mtilek->val = reg_l(s->src1.reg);
  mp_set_dirty();
}

def_EHelper(msettileki) {
  require_matrix();
  mtilek->val = s->src2.imm;
  mp_set_dirty();
}

def_EHelper(msettilen) {
  require_matrix();
  mtilen->val = reg_l(s->src1.reg);
  mp_set_dirty();
}

def_EHelper(msettileni) {
  require_matrix();
  mtilen->val = s->src2.imm;
  mp_set_dirty();
}

def_EHelper(msetcfg) {
  require_matrix();
  int idx = s->dest.reg;
  if (idx >= 8) {
    longjmp_exception(EX_II);
  }
  cpu.mcfg[idx].val = reg_l(s->src1.reg);
  mp_set_dirty();
}

def_EHelper(mgetcfg) {
  require_matrix();
  int idx = s->src1.reg;
  if (idx >= 8) {
    longjmp_exception(EX_II);
  }
  reg_l(s->dest.reg) = cpu.mcfg[idx].val;
}

def_EHelper(msyncreset) {
  require_matrix();
  int tok_i = (int)s->src2.imm;
  if (tok_i < 0 || tok_i >= MSYNC) {
    longjmp_exception(EX_II);
  }
  cpu.mtokr[tok_i] = 0;
#ifdef CONFIG_SHARE_REF
  msync_queue_emplace(0, tok_i);
#endif // CONFIG_SHARE_REF
  mp_set_dirty();
}

def_EHelper(mrelease) {
  require_matrix();
  int tok_i = (int)s->src2.imm;
  if (tok_i < 0 || tok_i >= MSYNC) {
    longjmp_exception(EX_II);
  }
#ifndef CONFIG_SHARE
  cpu.mtokr[tok_i]++;
#endif
#ifdef CONFIG_DIFFTEST_AMU_CTRL
  amu_ctrl_queue_mrelease_emplace(tok_i);
#endif // CONFIG_DIFFTEST_AMU_CTRL
#ifdef CONFIG_SHARE_CTRL
  cutest_mrelease_emplace(tok_i);
#endif // CONFIG_SHARE_CTRL
#ifndef CONFIG_SHARE
  mstore_queue_update_mrelease(tok_i, cpu.mtokr[tok_i]);
#endif // CONFIG_SHARE
  mp_set_dirty();
}

def_EHelper(macquire) {
  require_matrix();
  int tok_i = (int)s->src2.imm;
  if (tok_i < 0 || tok_i >= MSYNC) {
    longjmp_exception(EX_II);
  }
  // Do nothing in NEMU.
#ifndef CONFIG_SHARE
  Assert(cpu.mtokr[tok_i] >= reg_l(s->src1.reg),
    "Value(%ld) in msync register %d is not enough.", reg_l(s->src1.reg), tok_i);
#elif defined(CONFIG_SHARE_REF)
  if (cpu.mtokr[tok_i] < reg_l(s->src1.reg)) {
    Log("Value(%ld) in msync register %d is not enough.", reg_l(s->src1.reg), tok_i);
  }
#else // controller mode
  nemu_state.state = NEMU_WAIT;
  nemu_state.wait_r = tok_i;
  nemu_state.wait_val = reg_l(s->src1.reg);
#endif
#ifdef CONFIG_SHARE_REF
  msync_queue_emplace(1, tok_i);
#endif // CONFIG_SHARE_REF
  mstore_queue_update_acquire(tok_i, reg_l(s->src1.reg));
  mp_set_dirty();
}

def_EHelper(mfence)  {
  require_matrix();
#ifdef CONFIG_SHARE_REF
  msync_queue_emplace(2, 0);
#endif // CONFIG_SHARE_REF
  // Do nothing in NEMU.
}

#endif // CONFIG_RV_AME
