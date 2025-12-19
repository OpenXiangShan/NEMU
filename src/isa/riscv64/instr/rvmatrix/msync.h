#include <common.h>

#ifdef CONFIG_RVMATRIX

#include "cpu/exec.h"
#include "ext/amu_ctrl_queue_wrapper.h"
#include <ext/cutest.h>
#include "ext/msync_queue_wrapper.h"
#include "ext/mstore_queue_wrapper.h"
#include "../local-include/csr.h"
#include "../local-include/intr.h"
#include "../local-include/rtl.h"
#include "../local-include/reg.h"
#include <stdio.h>

def_EHelper(msyncreset) {
  cpu.mtokr[s->src2.imm] = 0;
  msync_queue_emplace(0, s->src2.imm);
}

def_EHelper(mrelease) {
  cpu.mtokr[s->src2.imm]++;
#ifdef CONFIG_DIFFTEST_AMU_CTRL
  amu_ctrl_queue_mrelease_emplace(s->src2.imm);
#endif // CONFIG_DIFFTEST_AMU_CTRL
#ifdef CONFIG_SHARE_CTRL
  cutest_mrelease_emplace(s->src2.imm);
#endif // CONFIG_SHARE_CTRL
  mstore_queue_update_mrelease(s->src2.imm, cpu.mtokr[s->src2.imm]);
}

def_EHelper(macquire) {
  // Do nothing in NEMU.
  Assert(cpu.mtokr[s->src2.imm] >= reg_l(s->src1.reg), "Value in token register is not enough.");
  msync_queue_emplace(1, s->src2.imm);
  mstore_queue_update_acquire(s->src2.imm, reg_l(s->src1.reg));
}

#endif // CONFIG_RVMATRIX
