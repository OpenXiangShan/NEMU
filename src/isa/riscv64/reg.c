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

const char *fpregsl[] = {
  "ft0", "ft1", "ft2", "ft3", "ft4", "ft5", "ft6", "ft7",
  "fs0", "fs1", "fa0", "fa1", "fa2", "fa3", "fa4", "fa5",
  "fa6", "fa7", "fs2", "fs3", "fs4", "fs5", "fs6", "fs7",
  "fs8", "fs9", "fs10", "fs11", "ft8", "ft9", "ft10", "ft11"
};

void isa_reg_display() {
  int i;
  for (i = 0; i < 32; i ++) {
    printf("%4s: " FMT_WORD " ", regsl[i], cpu.gpr[i]._64);
    if (i % 4 == 3) {
      printf("\n");
    }
  }
  for (i = 0; i < 32; i ++) {
    printf("%4s: " FMT_WORD " ", fpregsl[i], cpu.fpr[i]._64);
    if (i % 4 == 3) {
      printf("\n");
    }
  }
  printf("pc: " FMT_WORD " mstatus: " FMT_WORD " mcause: " FMT_WORD " mepc: " FMT_WORD "\n",
      cpu.pc, mstatus->val, mcause->val, mepc->val);
  rtlreg_t temp;
  csr_read(&temp, 0x100); // sstatus
  printf("%22s sstatus: " FMT_WORD " scause: " FMT_WORD " sepc: " FMT_WORD "\n",
      "", temp, scause->val, sepc->val);
  printf("satp: " FMT_WORD "\n", satp->val);
  printf("mip: " FMT_WORD " mie: " FMT_WORD " mscratch: " FMT_WORD " sscratch: " FMT_WORD "\n", 
      mip->val, mie->val, mscratch->val, sscratch->val);
  printf("mideleg: " FMT_WORD " medeleg: " FMT_WORD "\n", 
      mideleg->val, medeleg->val);
  printf("mtval: " FMT_WORD " stval: " FMT_WORD "mtvec: " FMT_WORD " stvec: " FMT_WORD "\n", 
      mtval->val, stval->val, mtvec->val, stvec->val);
  fflush(stdout);
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

rtlreg_t csr_array[4096] = {};

#define CSRS_DEF(name, addr) \
  concat(name, _t)* const name = (void *)&csr_array[addr];
MAP(CSRS, CSRS_DEF)

#define CSRS_EXIST(name, addr) [addr] = 1,
static bool csr_exist[4096] = {
  MAP(CSRS, CSRS_EXIST)
};

rtlreg_t csr_perf;

static inline word_t* csr_decode(uint32_t addr) {
  assert(addr < 4096);
  // Skip CSR for perfcnt
  // TODO: dirty implementation
  if ((addr >= 0xb00 && addr <= 0xb1f) || (addr >= 0x320 && addr <= 0x33f)) {
    return &csr_perf;
  }
  Assert(csr_exist[addr], "unimplemented CSR 0x%x at pc = " FMT_WORD, addr, cpu.pc);
  return &csr_array[addr];
}

#define SSTATUS_WMASK ((1 << 19) | (1 << 18) | (0x3 << 13) | (1 << 8) | (1 << 5) | (1 << 1))
#define SSTATUS_RMASK (SSTATUS_WMASK | (0x3 << 15) | (1ull << 63) | (3ull << 32))
#define SIE_MASK (0x222 & mideleg->val)
#define SIP_MASK (0x222 & mideleg->val)

#define FFLAGS_MASK 0x1f
#define FRM_MASK 0x03
#define FCSR_MASK 0xff

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
  } else if (src == (void *)fflags) {
    *dest = fflags->val & FFLAGS_MASK;
  } else if (src == (void *)frm) {
    *dest = frm->val & FRM_MASK;
  } else if (src == (void *)fcsr) {
    *dest = fcsr->val & FCSR_MASK;
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
    *dest = *src & 0xf3ff;
  } else if (dest == (void *)mideleg) {
    *dest = *src & 0x222;
  } else if (dest == (void *)fflags) {
    mstatus->fs = 3;
    mstatus->sd = 1;
    *dest = *src & FFLAGS_MASK;
    fcsr->val = (frm->val)<<5 | fflags->val;
  } else if (dest == (void *)frm) {
    mstatus->fs = 3;
    mstatus->sd = 1;
    *dest = *src & FRM_MASK;
    fcsr->val = (frm->val)<<5 | fflags->val;
  } else if (dest == (void *)fcsr) {
    mstatus->fs = 3;
    mstatus->sd = 1;
    *dest = *src & FCSR_MASK;
    fflags->val = *src & FFLAGS_MASK;
    frm->val = ((*src)>>5) & FRM_MASK;
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
