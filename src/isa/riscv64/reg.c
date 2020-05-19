#include <isa.h>
#include <monitor/difftest.h>
#include "local-include/reg.h"
#include "local-include/csr.h"
#include "local-include/intr.h"

const char *regsl[] = {
  "$0", "ra", "sp", "gp", "tp", "t0", "t1", "t2",
  "s0", "s1", "a0", "a1", "a2", "a3", "a4", "a5",
  "a6", "a7", "s2", "s3", "s4", "s5", "s6", "s7",
  "s8", "s9", "s10", "s11", "t3", "t4", "t5", "t6"
};

void isa_reg_display() {
  int i;
  for (i = 0; i < 32; i ++) {
    printf("%s: " FMT_WORD " ", regsl[i], cpu.gpr[i]._64);
    if (i % 4 == 3) printf("\n");
  }
  printf("pc: " FMT_WORD " mstatus: " FMT_WORD " mcause: " FMT_WORD " mepc: " FMT_WORD "\n",
      cpu.pc, mstatus->val, mcause->val, mepc->val);
  rtlreg_t temp;
  csr_read(&temp, 0x100); // sstatus
  printf("%22s sstatus: " FMT_WORD " scause: " FMT_WORD " sepc: " FMT_WORD "\n",
      "", temp, scause->val, sepc->val);
}

rtlreg_t isa_reg_str2val(const char *s, bool *success) {
  int i;
  *success = true;
  for (i = 0; i < 32; i ++) {
    if (strcmp(regsl[i], s) == 0) return reg_l(i);
  }

  if (strcmp("pc", s) == 0) return cpu.pc;

  *success = false;
  return 0;
}

static word_t csr_array[4096] = {};

#define CSRS_DEF(name, addr) \
  concat(name, _t)* const name = (void *)&csr_array[addr];
MAP(CSRS, CSRS_DEF)

#define CSRS_EXIST(name, addr) [addr] = 1,
static bool csr_exist[4096] = {
  MAP(CSRS, CSRS_EXIST)
};

static inline word_t* csr_decode(uint32_t addr) {
  assert(addr < 4096);
  Assert(csr_exist[addr], "unimplemented CSR 0x%x at pc = " FMT_WORD, addr, cpu.pc);
  return &csr_array[addr];
}

#define SSTATUS_WMASK ((1 << 19) | (1 << 18) | (0x3 << 13) | (1 << 8) | (1 << 5) | (1 << 1))
#define SSTATUS_RMASK (SSTATUS_WMASK | (0x3 << 15) | (1ull << 63) | (3ull << 32))
#define SIE_MASK (0x222 & mideleg->val)
#define SIP_MASK (0x222 & mideleg->val)

void csr_read(rtlreg_t *dest, uint32_t addr) {
  word_t *src = csr_decode(addr);
#ifndef __DIFF_REF_NEMU__
  difftest_skip_dut(1, 3);
#endif

  if (src == (void *)sstatus) {
    *dest = mstatus->val & SSTATUS_RMASK;
  } else if (src == (void *)sie) {
    *dest = mie->val & SIE_MASK;
  } else if (src == (void *)sip) {
    *dest = mip->val & SIP_MASK;
  } else {
    *dest = *src;
  }
}

void csr_write(uint32_t addr, rtlreg_t *src) {
  word_t *dest = csr_decode(addr);
  if (dest == (void *)sstatus) {
    mstatus->val = (mstatus->val & ~SSTATUS_WMASK) | (*src & SSTATUS_WMASK);
  } else if (dest == (void *)sie) {
    mie->val = (mie->val & ~SIE_MASK) | (*src & SIE_MASK);
  } else if (dest == (void *)sip) {
    mip->val = (mip->val & ~SIP_MASK) | (*src & SIP_MASK);
  } else if (dest == (void *)medeleg) {
    *dest = *src & 0xbbff;
  } else if (dest == (void *)mideleg) {
    *dest = *src & 0x222;
  } else {
    *dest = *src;
  }

  if (dest == (void *)sstatus || dest == (void *)mstatus) {
#ifdef __DIFF_REF_QEMU__
    // mstatus.fs is always dirty or off in QEMU 3.1.0
    if (mstatus->fs) { mstatus->fs = 3; }
#endif
    mstatus->sd = (mstatus->fs == 3);
  }
}

void change_mode(uint8_t m) {
  assert(m < 4 && m != MODE_H);
  cpu.mode = m;
}
