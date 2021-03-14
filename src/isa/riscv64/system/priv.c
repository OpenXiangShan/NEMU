#include "../local-include/csr.h"
#include "../local-include/rtl.h"
#include "../local-include/intr.h"
#include <cpu/tcache.h>
#include <cpu/cpu.h>

int update_mmu_state();

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

#define FFLAGS_MASK 0x1f
#define FRM_MASK 0x03
#define FCSR_MASK 0xff

static inline word_t csr_read(word_t *src) {
  if (src == (void *)sstatus) {
    return mstatus->val & SSTATUS_RMASK;
  } else if (src == (void *)sie) {
    return mie->val & SIE_MASK;
  } else if (src == (void *)sip) {
    return mip->val & SIP_MASK;
  } else if (src == (void *)fflags) {
    return fflags->val & FFLAGS_MASK;
  } else if (src == (void *)frm) {
    return frm->val & FRM_MASK;
  } else if (src == (void *)fcsr) {
    return fcsr->val & FCSR_MASK;
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
  } else if (dest == (void *)fflags) {
    mstatus->fs = 3;
    mstatus->sd = 1;
    *dest = src & FFLAGS_MASK;
    fcsr->val = (frm->val)<<5 | fflags->val;
  } else if (dest == (void *)frm) {
    mstatus->fs = 3;
    mstatus->sd = 1;
    *dest = src & FRM_MASK;
    fcsr->val = (frm->val)<<5 | fflags->val;
  } else if (dest == (void *)fcsr) {
    mstatus->fs = 3;
    mstatus->sd = 1;
    *dest = src & FCSR_MASK;
    fflags->val = src & FFLAGS_MASK;
    frm->val = ((src)>>5) & FRM_MASK;
  } else {
    *dest = src;
  }

  if (dest == (void *)sstatus || dest == (void *)mstatus) {
    // mstatus.fs is always dirty or off in QEMU 3.1.0
    if (ISDEF(CONFIG_DIFFTEST_REF_QEMU) && mstatus->fs) { mstatus->fs = 3; }
    mstatus->sd = (mstatus->fs == 3);
  }

  if (dest == (void *)mstatus || dest == (void *)satp) {
    if (update_mmu_state()) set_system_state_update_flag();
  }
}

word_t csrid_read(uint32_t csrid) {
  return csr_read(csr_decode(csrid));
}

static void csrrw(rtlreg_t *dest, const rtlreg_t *src, uint32_t csrid) {
  if (csrid == 0xc01) { longjmp_exception(EX_II); } // time
  word_t *csr = csr_decode(csrid);
  word_t tmp = (src != NULL ? *src : 0);
  if (dest != NULL) { *dest = csr_read(csr); }
  if (src != NULL) { csr_write(csr, tmp); }
}

static word_t priv_instr(uint32_t op, const rtlreg_t *src) {
  switch (op) {
    case 0x102: // sret
      mstatus->sie = mstatus->spie;
      mstatus->spie = (ISDEF(CONFIG_DIFFTEST_REF_QEMU) ? 0 // this is bug of QEMU
          : 1);
      cpu.mode = mstatus->spp;
      mstatus->spp = MODE_U;
      return sepc->val;
    case 0x302: // mret
      mstatus->mie = mstatus->mpie;
      mstatus->mpie = (ISDEF(CONFIG_DIFFTEST_REF_QEMU) ? 0 // this is bug of QEMU
          : 1);
      cpu.mode = mstatus->mpp;
      mstatus->mpp = MODE_U;
      update_mmu_state();
      return mepc->val;
      break;
    case 0x120: // sfence.vma
      tcache_flush();
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
