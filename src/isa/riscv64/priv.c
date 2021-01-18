#include <monitor/difftest.h>
#include "local-include/csr.h"
#include "local-include/rtl.h"
#include "local-include/intr.h"

#define INTR_BIT (1ULL << 63)
enum {
  IRQ_USIP, IRQ_SSIP, IRQ_HSIP, IRQ_MSIP,
  IRQ_UTIP, IRQ_STIP, IRQ_HTIP, IRQ_MTIP,
  IRQ_UEIP, IRQ_SEIP, IRQ_HEIP, IRQ_MEIP
};

static inline void change_mode(uint8_t m) {
  assert(m < 4 && m != MODE_H);
  cpu.mode = m;
}

word_t raise_intr(word_t NO, vaddr_t epc) {
  // TODO: Trigger an interrupt/exception with ``NO''

  switch (NO) {
    case EX_II:
    case EX_IPF:
    case EX_LPF:
    case EX_SPF: difftest_skip_dut(1, 2); break;
  }

  word_t deleg = (NO & INTR_BIT ? mideleg->val : medeleg->val);
  bool delegS = ((deleg & (1 << (NO & 0xf))) != 0) && (cpu.mode < MODE_M);

  if (delegS) {
    scause->val = NO;
    sepc->val = epc;
    mstatus->spp = cpu.mode;
    mstatus->spie = mstatus->sie;
    mstatus->sie = 0;
    switch (NO) {
      case EX_IPF: case EX_LPF: case EX_SPF:
      case EX_LAM: case EX_SAM:
        break;
      default: stval->val = 0;
    }
    cpu.mode = MODE_S;
    return stvec->val;
  } else {
    mcause->val = NO;
    mepc->val = epc;
    mstatus->mpp = cpu.mode;
    mstatus->mpie = mstatus->mie;
    mstatus->mie = 0;
    switch (NO) {
      case EX_IPF: case EX_LPF: case EX_SPF:
      case EX_LAM: case EX_SAM:
        break;
      default: mtval->val = 0;
    }
    cpu.mode = MODE_M;
    return mtvec->val;
  }
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

static inline word_t csr_read(word_t *src) {
  if (src == (void *)sstatus) {
    return mstatus->val & SSTATUS_RMASK;
  } else if (src == (void *)sie) {
    return mie->val & SIE_MASK;
  } else if (src == (void *)sip) {
    return mip->val & SIP_MASK;
  }
  return *src;
}

static inline void csr_write(word_t *dest, word_t src) {
  if (dest == (void *)sstatus) {
    mstatus->val = (mstatus->val & ~SSTATUS_WMASK) | (src & SSTATUS_WMASK);
  } else if (dest == (void *)sie) {
    mie->val = (mie->val & ~SIE_MASK) | (src & SIE_MASK);
  } else if (dest == (void *)sip) {
    mip->val = (mip->val & ~SIP_MASK) | (src & SIP_MASK);
  } else if (dest == (void *)medeleg) {
    *dest = src & 0xbbff;
  } else if (dest == (void *)mideleg) {
    *dest = src & 0x222;
  } else {
    *dest = src;
  }

  if (dest == (void *)sstatus || dest == (void *)mstatus) {
#ifdef __DIFF_REF_QEMU__
    // mstatus.fs is always dirty or off in QEMU 3.1.0
    if (mstatus->fs) { mstatus->fs = 3; }
#endif
    mstatus->sd = (mstatus->fs == 3);
  }
}

word_t csrid_read(uint32_t csrid) {
  return csr_read(csr_decode(csrid));
}

static void csrrw(rtlreg_t *dest, const rtlreg_t *src, uint32_t csrid) {
  word_t *csr = csr_decode(csrid);
  word_t tmp = (src != NULL ? *src : 0);
  if (dest != NULL) { *dest = csr_read(csr); }
  if (src != NULL) { csr_write(csr, tmp); }
}

static word_t priv_instr(uint32_t op, const rtlreg_t *src) {
  switch (op) {
    case 0x102: // sret
      mstatus->sie = mstatus->spie;
#ifdef __DIFF_REF_QEMU__
      // this is bug of QEMU
      mstatus->spie = 0;
#else
      mstatus->spie = 1;
#endif
      change_mode(mstatus->spp);
      mstatus->spp = MODE_U;
      return sepc->val;
    case 0x302: // mret
      mstatus->mie = mstatus->mpie;
#ifdef __DIFF_REF_QEMU__
      // this is bug of QEMU
      mstatus->mpie = 0;
#else
      mstatus->mpie = 1;
#endif
      change_mode(mstatus->mpp);
      mstatus->mpp = MODE_U;
      return mepc->val;
      break;
    default: panic("Unsupported privilige operation = %d", op);
  }
  return 0;
}

void isa_hostcall(uint32_t id, rtlreg_t *dest, const rtlreg_t *src, uint32_t imm) {
  word_t ret = 0;
  switch (id) {
    case HOSTCALL_CSR: csrrw(dest, src, imm); return;
    case HOSTCALL_TRAP: ret = raise_intr(imm, *src); break;
    case HOSTCALL_PRIV: ret = priv_instr(imm, src); break;
    default: panic("Unsupported hostcall ID = %d", id);
  }
  if (dest) *dest = ret;
}

void query_intr() {
  word_t intr_vec = mie->val & mip->val;
  if (!intr_vec) return;

  const int priority [] = {
    IRQ_MEIP, IRQ_MSIP, IRQ_MTIP,
    IRQ_SEIP, IRQ_SSIP, IRQ_STIP,
    IRQ_UEIP, IRQ_USIP, IRQ_UTIP
  };
  int i;
  for (i = 0; i < 9; i ++) {
    int irq = priority[i];
    if (intr_vec & (1 << irq)) {
      bool deleg = (mideleg->val & (1 << irq)) != 0;
      bool global_enable = (deleg ? ((cpu.mode == MODE_S) && mstatus->sie) || (cpu.mode < MODE_S) :
          ((cpu.mode == MODE_M) && mstatus->mie) || (cpu.mode < MODE_M));
      if (global_enable) {
        cpu.pc = raise_intr(irq | INTR_BIT, cpu.pc);
      }
    }
  }
}
