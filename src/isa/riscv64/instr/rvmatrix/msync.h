#include <common.h>

#ifdef CONFIG_RVMATRIX

#include "cpu/exec.h"
#include "../local-include/csr.h"
#include "../local-include/intr.h"
#include "../local-include/rtl.h"
#include "../local-include/reg.h"
#include <stdio.h>

def_EHelper(msyncreset) {
  cpu.mtokr[s->src2.imm] = 0;
}

def_EHelper(mrelease) {
  cpu.mtokr[s->src2.imm]++;
}

def_EHelper(macquire) {
  // Do nothing in NEMU.
  Assert(cpu.mtokr[s->src2.imm] >= reg_l(s->src1.reg), "Value in token register is not enough.");
}

#endif // CONFIG_RVMATRIX