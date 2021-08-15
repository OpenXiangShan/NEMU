#include <rtl/rtl.h>

#ifndef __ICS_EXPORT
#include "../local-include/csr.h"
#include <cpu/cpu.h>
#include <cpu/difftest.h>

int update_mmu_state();
uint64_t clint_uptime();
void fp_set_dirty();
void fp_update_rm_cache(uint32_t rm);

static word_t csr_array[4096] = {};

#define CSRS_DEF(name, addr) \
  concat(name, _t)* const name = (concat(name, _t) *)&csr_array[addr];
MAP(CSRS, CSRS_DEF)

#define CSRS_EXIST(name, addr) csr_exist[addr] = 1;
static bool csr_exist[4096] = {};
void init_csr() {
  MAP(CSRS, CSRS_EXIST)
};

static word_t* csr_decode(uint32_t addr) {
  assert(addr < 4096);
  Assert(csr_exist[addr], "unimplemented CSR 0x%x at pc = " FMT_WORD, addr, cpu.pc);
  return &csr_array[addr];
}

#define SSTATUS_WMASK ((1 << 19) | (1 << 18) | (0x3 << 13) | (1 << 8) | (1 << 5) | (1 << 1))
#define SSTATUS_RMASK (SSTATUS_WMASK | (0x3 << 15) | (1ull << 63) | (3ull << 32))
#define SIE_MASK (0x222 & mideleg->val)
#define SIP_MASK (0x222 & mideleg->val)
#define FCSR_MASK 0xff

#define is_read(csr) (src == (void *)(csr))
#define is_write(csr) (dest == (void *)(csr))
#define mask_bitset(old, mask, new) (((old) & ~(mask)) | ((new) & (mask)))

static void update_mstatus_sd() {
  // mstatus.fs is always dirty or off in QEMU 3.1.0
  if (ISDEF(CONFIG_DIFFTEST_REF_QEMU) && mstatus->fs) { mstatus->fs = 3; }
  mstatus->sd = (mstatus->fs == 3);
}

static word_t csr_read(word_t *src) {
  if (is_read(mstatus) || is_read(sstatus)) { update_mstatus_sd(); }

  if (is_read(sstatus))     { return mstatus->val & SSTATUS_RMASK; }
  else if (is_read(sie))    { return mie->val & SIE_MASK; }
  else if (is_read(sip))    { difftest_skip_ref(); return mip->val & SIP_MASK; }
  else if (is_read(fcsr))   { return fcsr->val & FCSR_MASK; }
  else if (is_read(fflags)) { return fcsr->fflags.val; }
  else if (is_read(frm))    { return fcsr->frm; }
  else if (is_read(mtime))  { difftest_skip_ref(); return MUXDEF(CONFIG_PA, 0, clint_uptime()); }
  if (is_read(mip)) { difftest_skip_ref(); }
  return *src;
}

static void csr_write(word_t *dest, word_t src) {
  if (is_write(sstatus)) { mstatus->val = mask_bitset(mstatus->val, SSTATUS_WMASK, src); }
  else if (is_write(sie)) { mie->val = mask_bitset(mie->val, SIE_MASK, src); }
  else if (is_write(sip)) { mip->val = mask_bitset(mip->val, SIP_MASK, src); }
  else if (is_write(medeleg)) { *dest = src & 0xbbff; }
  else if (is_write(mideleg)) { *dest = src & 0x222; }
  else if (is_write(fflags)) { fcsr->fflags.val = src; }
  else if (is_write(frm)) { fcsr->frm = src; }
  else if (is_write(fcsr)) { *dest = src & FCSR_MASK; }
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

void isa_hostcall(uint32_t id, rtlreg_t *dest,
    const rtlreg_t *src1, const rtlreg_t *src2, word_t imm) {
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
    case HOSTCALL_TRAP: ret = isa_raise_intr(imm, *src1); break;
#endif
    case HOSTCALL_PRIV: ret = priv_instr(imm, src1); break;
    default: panic("Unsupported hostcall ID = %d", id);
  }
  if (dest) *dest = ret;
}
#else
void isa_hostcall(uint32_t id, rtlreg_t *dest,
    const rtlreg_t *src1, const rtlreg_t *src2, word_t imm) {
  word_t ret = 0;
  switch (id) {
    default: panic("Unsupported hostcall ID = %d", id);
  }
  if (dest) *dest = ret;
}
#endif
