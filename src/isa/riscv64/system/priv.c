#include "../local-include/csr.h"
#include "../local-include/rtl.h"
#include "../local-include/intr.h"
#include <cpu/cpu.h>
#include <cpu/difftest.h>

int update_mmu_state();
uint64_t clint_uptime();
void fp_set_dirty();
void fp_update_rm_cache(uint32_t rm);

rtlreg_t csr_array[4096] = {};

#define CSRS_DEF(name, addr) \
  concat(name, _t)* const name = (concat(name, _t) *)&csr_array[addr];
MAP(CSRS, CSRS_DEF)
#ifdef CONFIG_RVV_010
MAP(VCSRS, CSRS_DEF)
#endif // CONFIG_RVV_010

#define CSRS_EXIST(name, addr) csr_exist[addr] = 1;
static bool csr_exist[4096] = {};
void init_csr() {
  MAP(CSRS, CSRS_EXIST)
  #ifdef CONFIG_RVV_010
  MAP(VCSRS, CSRS_EXIST)
  #endif // CONFIG_RVV_010
};

rtlreg_t csr_perf;

static inline bool csr_is_legal(uint32_t addr) {
  assert(addr < 4096);
  // CSR does not exist
  if(!csr_exist[addr]) {
#ifdef CONFIG_PANIC_ON_UNIMP_CSR
    panic("[NEMU] unimplemented CSR 0x%x", addr);
#endif
    return false;
  }
  // CSR exists, but access is not legal
  int lowest_access_priv_level = (addr & 0b11 << 8) >> 8; // addr(9,8)
  if (!(cpu.mode >= lowest_access_priv_level)) {
    return false;
  }
  return true;
}

static inline word_t* csr_decode(uint32_t addr) {
  assert(addr < 4096);
  // Now we check if CSR is implemented / legal to access in csr_is_legal()
  // Assert(csr_exist[addr], "unimplemented CSR 0x%x at pc = " FMT_WORD, addr, cpu.pc);

  // Skip CSR for perfcnt
  // TODO: dirty implementation
  if ((addr >= 0xb00 && addr <= 0xb1f) || (addr >= 0x320 && addr <= 0x33f)) {
    return &csr_perf;
  }
  return &csr_array[addr];
}
#ifdef CONFIG_RVV_010
#define SSTATUS_WMASK ((1 << 19) | (1 << 18) | (0x3 << 13) | (0x3 << 9) | (1 << 8) | (1 << 5) | (1 << 1))
#else
#define SSTATUS_WMASK ((1 << 19) | (1 << 18) | (0x3 << 13) | (1 << 8) | (1 << 5) | (1 << 1))
#endif // CONFIG_RVV_010
#define SSTATUS_RMASK (SSTATUS_WMASK | (0x3 << 15) | (1ull << 63) | (3ull << 32))
#define SIE_MASK (0x222 & mideleg->val)
#define SIP_MASK (0x222 & mideleg->val)

#define FFLAGS_MASK 0x1f
#define FRM_MASK 0x07
#define FCSR_MASK 0xff

#define is_read(csr) (src == (void *)(csr))
#define is_write(csr) (dest == (void *)(csr))
#define mask_bitset(old, mask, new) (((old) & ~(mask)) | ((new) & (mask)))

static inline void update_mstatus_sd() {
  // mstatus.fs is always dirty or off in QEMU 3.1.0
  if (ISDEF(CONFIG_DIFFTEST_REF_QEMU) && mstatus->fs) { mstatus->fs = 3; }
  mstatus->sd = (mstatus->fs == 3);
}

static inline word_t csr_read(word_t *src) {
  if (is_read(mstatus) || is_read(sstatus)) { update_mstatus_sd(); }

  if (is_read(sstatus))     { return mstatus->val & SSTATUS_RMASK; }
  else if (is_read(sie))    { return mie->val & SIE_MASK; }
  else if (is_read(sip))    { difftest_skip_ref(); return mip->val & SIP_MASK; }
  else if (is_read(fcsr))   { return fcsr->val & FCSR_MASK; }
  else if (is_read(fflags)) { return fcsr->fflags.val & FFLAGS_MASK; }
  else if (is_read(frm))    { return fcsr->frm & FRM_MASK; }
#ifndef CONFIG_SHARE
  else if (is_read(mtime))  { difftest_skip_ref(); return clint_uptime(); }
#endif
  if (is_read(mip)) { difftest_skip_ref(); }
  return *src;
}

#ifdef CONFIG_RVV_010
void vcsr_write(uint32_t addr,  rtlreg_t *src) {
  word_t *dest = csr_decode(addr);
  *dest = *src;
}
#endif // CONFIG_RVV_010

static inline void csr_write(word_t *dest, word_t src) {
  if (is_write(sstatus)) { mstatus->val = mask_bitset(mstatus->val, SSTATUS_WMASK, src); }
  else if (is_write(sie)) { mie->val = mask_bitset(mie->val, SIE_MASK, src); }
  else if (is_write(sip)) { mip->val = mask_bitset(mip->val, SIP_MASK, src); }
  else if (is_write(medeleg)) { *dest = src & 0xf3ff; }
  else if (is_write(mideleg)) { *dest = src & 0x222; }
  else if (is_write(fflags)) {
    *dest = src & FFLAGS_MASK;
    fcsr->val = (frm->val)<<5 | fflags->val;
    // fcsr->fflags.val = src;
  }
  else if (is_write(frm)) {
    *dest = src & FRM_MASK;
    fcsr->val = (frm->val)<<5 | fflags->val;
    // fcsr->frm = src;
  }
  else if (is_write(fcsr)) {
    *dest = src & FCSR_MASK;
    fflags->val = src & FFLAGS_MASK;
    frm->val = ((src)>>5) & FRM_MASK;
    // *dest = src & FCSR_MASK;
  }
  else { *dest = src; }

  bool need_update_mstatus_sd = false;
  if (is_write(fflags) || is_write(frm) || is_write(fcsr)) {
    fp_set_dirty();
    fp_update_rm_cache(fcsr->frm);
    need_update_mstatus_sd = true;
  }

  if (is_write(sstatus) || is_write(mstatus) || need_update_mstatus_sd) {
    update_mstatus_sd();
  }

  if (is_write(mstatus) || is_write(satp)) { update_mmu_state(); }
  if (is_write(mstatus) || is_write(sstatus) || is_write(satp) ||
      is_write(mie) || is_write(sie) || is_write(mip) || is_write(sip)) {
    set_sys_state_flag(SYS_STATE_UPDATE);
  }
}

word_t csrid_read(uint32_t csrid) {
  return csr_read(csr_decode(csrid));
}

static void csrrw(rtlreg_t *dest, const rtlreg_t *src, uint32_t csrid) {
  if (!csr_is_legal(csrid)) {
    longjmp_exception(EX_II);
    return;
  }
  word_t *csr = csr_decode(csrid);
  word_t tmp = (src != NULL ? *src : 0);
  if (dest != NULL) { *dest = csr_read(csr); }
  if (src != NULL) { csr_write(csr, tmp); }
}

static word_t priv_instr(uint32_t op, const rtlreg_t *src) {
  switch (op) {
#ifndef CONFIG_MODE_USER
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
      mmu_tlb_flush(*src);
      break;
    case 0x105: break; // wfi
#endif
    case -1: // fence.i
      set_sys_state_flag(SYS_STATE_FLUSH_TCACHE);
      break;
    default: panic("Unsupported privilige operation = %d", op);
  }
  return 0;
}

void isa_hostcall(uint32_t id, rtlreg_t *dest, const rtlreg_t *src1,
    const rtlreg_t *src2, word_t imm) {
  word_t ret = 0;
  switch (id) {
    case HOSTCALL_CSR: csrrw(dest, src1, imm); return;
#ifdef CONFIG_MODE_USER
    case HOSTCALL_TRAP:
      Assert(imm == 0x8, "Unsupport exception = %ld", imm);
      uintptr_t host_syscall(uintptr_t id, uintptr_t arg1, uintptr_t arg2,
          uintptr_t arg3, uintptr_t arg4, uintptr_t arg5, uintptr_t arg6);
      cpu.gpr[10]._64 = host_syscall(cpu.gpr[17]._64, cpu.gpr[10]._64, cpu.gpr[11]._64,
          cpu.gpr[12]._64, cpu.gpr[13]._64, cpu.gpr[14]._64, cpu.gpr[15]._64);
      ret = *src1 + 4;
      break;
#else
    case HOSTCALL_TRAP: ret = raise_intr(imm, *src1); break;
#endif
    case HOSTCALL_PRIV: ret = priv_instr(imm, src1); break;
    default: panic("Unsupported hostcall ID = %d", id);
  }
  if (dest) *dest = ret;
}
