#include <common.h>

#ifdef CONFIG_RVMATRIX

#include "cpu/exec.h"
#include "ext/msync_queue_wrapper.h"
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
  amu_ctrl_queue_mrelease_emplace(s->src2.imm);
}

def_EHelper(macquire) {
  // Do nothing in NEMU.
  Assert(cpu.mtokr[s->src2.imm] >= reg_l(s->src1.reg), "Value in token register is not enough.");
  msync_queue_emplace(1, s->src2.imm);
}

#endif // CONFIG_RVMATRIX
