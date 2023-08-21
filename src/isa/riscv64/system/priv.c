/***************************************************************************************
* Copyright (c) 2014-2021 Zihao Yu, Nanjing University
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

#include "../local-include/csr.h"
#include "../local-include/rtl.h"
#include "../local-include/intr.h"
#include <cpu/cpu.h>
#include <cpu/difftest.h>
#include <memory/paddr.h>
#include <isa.h>

int update_mmu_state();
uint64_t clint_uptime();
void fp_set_dirty();
void fp_update_rm_cache(uint32_t rm);
void vp_set_dirty();

rtlreg_t csr_array[4096] = {};

#define CSRS_DEF(name, addr) \
  concat(name, _t)* const name = (concat(name, _t) *)&csr_array[addr];

MAP(CSRS, CSRS_DEF)
#ifdef CONFIG_RVV
  MAP(VCSRS, CSRS_DEF)
#endif // CONFIG_RVV
#ifdef CONFIG_RVN
  MAP(NCSRS, CSRS_DEF)
#endif // CONFIG_RVN
#ifdef CONFIG_RV_ARCH_CSRS
  MAP(ARCH_CSRS, CSRS_DEF)
#endif // CONFIG_RV_ARCH_CSRS
#ifdef CONFIG_RV_DASICS
  MAP(DASICS_CSRS, CSRS_DEF)
#endif  // CONFIG_RV_DASICS

#define CSRS_EXIST(name, addr) csr_exist[addr] = 1;
static bool csr_exist[4096] = {};
void init_csr() {
  MAP(CSRS, CSRS_EXIST)
  #ifdef CONFIG_RVN
  MAP(NCSRS, CSRS_EXIST)
  #endif  // CONFIG_RVN
  #ifdef CONFIG_RVV
  MAP(VCSRS, CSRS_EXIST)
  #endif // CONFIG_RVV
  #ifdef CONFIG_RV_ARCH_CSRS
  MAP(ARCH_CSRS, CSRS_EXIST)
  #endif // CONFIG_RV_ARCH_CSRS
  #ifdef CONFIG_RV_DASICS
  MAP(DASICS_CSRS, CSRS_EXIST)
  #endif  // CONFIG_RV_DASICS
};

rtlreg_t csr_perf;

static inline bool csr_is_legal(uint32_t addr, bool need_write, vaddr_t pc) {
  assert(addr < 4096);
  // Attempts to access a non-existent CSR raise an illegal instruction exception.
  if(!csr_exist[addr]) {
#ifdef CONFIG_PANIC_ON_UNIMP_CSR
    panic("[NEMU] unimplemented CSR 0x%x", addr);
#endif
    return false;
  }
  // Attempts to access a CSR without appropriate privilege level
  int lowest_access_priv_level = (addr & 0b11 << 8) >> 8; // addr(9,8)
#ifdef CONFIG_RV_DASICS
  if (!(cpu.mode >= lowest_access_priv_level && dasics_in_trusted_zone(pc))) {
    panic("[NEMU] illegal csr access for dasics:0x%x mode: %ld pc: %lx", addr,cpu.mode,pc);
    return false;
  }
#else
  if (!(cpu.mode >= lowest_access_priv_level)) {
    panic("[NEMU] illegal csr access for dasics:0x%x mode:%ld", addr,cpu.mode);
    return false;
  }
#endif  // CONFIG_RV_DASICS
  // or to write a read-only register also raise illegal instruction exceptions.
  if (need_write && (addr >> 10) == 0x3) {
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

// WPRI, SXL, UXL cannot be written
#define MSTATUS_WMASK (0x7e79bbUL) | (1UL << 63)
#ifdef CONFIG_RVV
#define SSTATUS_WMASK ((1 << 19) | (1 << 18) | (0x3 << 13) | (0x3 << 9) | (1 << 8) | (1 << 5) | (1 << 1))
#else
#define SSTATUS_WMASK ((1 << 19) | (1 << 18) | (0x3 << 13) | (1 << 8) | (1 << 5) | (1 << 1))
#endif // CONFIG_RVV
#define SSTATUS_RMASK (SSTATUS_WMASK | (0x3 << 15) | (1ull << 63) | (3ull << 32))
#ifdef CONFIG_RVN
#define USTATUS_WMASK ((1 << 4) | 0x1)
#define USTATUS_RMASK USTATUS_WMASK
#endif  // CONFIG_RVN

#define MIP_MASK ((1 << 9) | (1 << 8) | (1 << 5) | (1 << 4) | (1 << 1) | (1 << 0))
#define SIE_MASK (0x222 & mideleg->val)
#define SIP_MASK (0x222 & mideleg->val)
#define SIP_WMASK_S 0x2
#define MTIE_MASK (1 << 7)
#ifdef CONFIG_RVN
#define UIE_MASK (0x111 & sideleg->val)
#define UIP_MASK (0x111 & sideleg->val)
#define UIP_WMASK_U 0x1
#endif  // CONFIG_RVN

#define FFLAGS_MASK 0x1f
#define FRM_MASK 0x07
#define FCSR_MASK 0xff
#define SATP_SV39_MASK 0xf000000000000000ULL
#define is_read(csr) (src == (void *)(csr))
#define is_write(csr) (dest == (void *)(csr))
#define is_read_pmpcfg (src >= &(csr_array[CSR_PMPCFG0]) && src < (&(csr_array[CSR_PMPCFG0]) + (MAX_NUM_PMP/4)))
#define is_read_pmpaddr (src >= &(csr_array[CSR_PMPADDR0]) && src < (&(csr_array[CSR_PMPADDR0]) + MAX_NUM_PMP))
#define is_write_pmpcfg (dest >= &(csr_array[CSR_PMPCFG0]) && dest < (&(csr_array[CSR_PMPCFG0]) + (MAX_NUM_PMP/4)))
#define is_write_pmpaddr (dest >= &(csr_array[CSR_PMPADDR0]) && dest < (&(csr_array[CSR_PMPADDR0]) + MAX_NUM_PMP))
#define is_write_dasics_mem_bound (dest >= &(csr_array[CSR_DLBOUND0]) && dest < (&(csr_array[CSR_DLBOUND0]) + MAX_DASICS_LIBBOUNDS))
#define is_write_dasics_jump_bound (dest >= &(csr_array[CSR_DJBOUND0]) && dest < (&(csr_array[CSR_DJBOUND0]) + MAX_DASICS_JUMPBOUNDS))
#define mask_bitset(old, mask, new) (((old) & ~(mask)) | ((new) & (mask)))

#ifdef CONFIG_RV_DASICS
#define DUMCFG_MASK (MCFG_UENA | MCFG_UCLS)
#define DSMCFG_MASK (MCFG_SENA | MCFG_SCLS | DUMCFG_MASK)
#define BOUND_ADDR_ALGIN 0x7
bool dasics_in_trusted_zone(uint64_t pc)
{
  bool is_smain_enable = dsmcfg->mcfg_sena;
  bool is_umain_enable = dsmcfg->mcfg_uena;

  bool in_smain_zone = pc >= dsmbound0->val && pc < dsmbound1->val && cpu.mode == MODE_S && is_smain_enable;
  bool in_umain_zone = pc >= dumbound0->val && pc < dumbound1->val && cpu.mode == MODE_U && is_umain_enable;

  bool in_s_trusted_zone = in_smain_zone || (cpu.mode == MODE_S && !is_smain_enable);
  bool in_u_trusted_zone = in_umain_zone || (cpu.mode == MODE_U && !is_umain_enable);

  return cpu.mode == MODE_M || in_s_trusted_zone || in_u_trusted_zone;
}

uint8_t dasics_libcfg_from_index(int i) {
  assert(0 <= i && i < MAX_DASICS_LIBBOUNDS);
  return (csr_array[CSR_DLCFG0] >> (i << 2)) & LIBCFG_MASK;
}

word_t dasics_libbound_from_index(int i) {
  assert(0 <= i && i < (MAX_DASICS_LIBBOUNDS << 1));
  return csr_array[CSR_DLBOUND0 + i];
}

uint16_t dasics_jumpcfg_from_index(int i) {
  assert(0 <= i && i < MAX_DASICS_JUMPBOUNDS);
  return (csr_array[CSR_DJCFG] >> (i << 4)) & JUMPCFG_MASK;
}

word_t dasics_jumpbound_low_from_index(int i) {
  assert(0 <= i && i < MAX_DASICS_JUMPBOUNDS);
  return csr_array[CSR_DJBOUND0 + 2*i];
}

word_t dasics_jumpbound_high_from_index(int i) {
  assert(0 <= i && i < MAX_DASICS_JUMPBOUNDS);
  return csr_array[CSR_DJBOUND0 + 2*i + 1];
}

bool dasics_match_dlib(uint64_t addr, uint8_t cfg)
{
  // Check whether the addr is within dlbounds which is marked as cfg
  bool within_range = false;
  for (int i = 0; i < MAX_DASICS_LIBBOUNDS; ++i) {
    uint8_t cfgval = dasics_libcfg_from_index(i);
    word_t boundlo = dasics_libbound_from_index(i << 1);
    word_t boundhi = dasics_libbound_from_index((i << 1) + 1);

    if (!((cfgval & cfg) ^ cfg) && boundlo <= addr && addr < boundhi) {
      within_range = true;
      break;
    }
  }

  return within_range;
}

bool dasics_match_djumpbound(uint64_t addr, uint8_t cfg) {
  bool within_range = false;
  for (int i = 0; i < MAX_DASICS_JUMPBOUNDS; ++i) {
    uint16_t cfgval = dasics_jumpcfg_from_index(i);
    word_t boundlo = dasics_jumpbound_low_from_index(i);
    word_t boundhi = dasics_jumpbound_high_from_index(i);
    Logm("[free zone check] cfgval:%d,  boundlo:%lx  boundhi:%lx\n",cfgval,boundlo,boundhi);

    if (!((cfgval & cfg) ^ cfg) && boundlo <= addr && addr < boundhi) {
      within_range = true;
      break;
    }
  }
  return within_range;
}

void dasics_ldst_helper(vaddr_t pc, vaddr_t vaddr, int len, int type) {
  // TODO: What about MEM_TYPE_IFETCH ???
  if (dasics_in_trusted_zone(pc)) {
    return;
  }

  if (type == MEM_TYPE_READ) {
    int ex = (cpu.mode == MODE_U) ? EX_DULAF : EX_DSLAF;
    for (int i = 0; i < len; i++) {
      if (!dasics_match_dlib(vaddr + i, LIBCFG_V | LIBCFG_R)) {
        INTR_TVAL_REG(ex) = vaddr + i;  // To avoid load inst that crosses libzone
        Logm("Dasics load exception occur %lx", vaddr);
        //isa_reg_display();
        longjmp_exception(ex);
        break;
      }
    }
  }
  else if (type == MEM_TYPE_WRITE) {
    int ex = (cpu.mode == MODE_U) ? EX_DUSAF : EX_DSSAF;
    for (int i = 0; i < len; ++i) {
      if (!dasics_match_dlib(vaddr + i, LIBCFG_V | LIBCFG_W)) {
        INTR_TVAL_REG(ex) = vaddr + i;  // To avoid store inst that crosses libzone
        Logm("Dasics store exception occur %lx", vaddr);
        //isa_reg_display();
        longjmp_exception(ex);
        break;
      }
    }
  }
}

void dasics_redirect_helper(vaddr_t pc, vaddr_t newpc, vaddr_t nextpc) {
  // Check whether this redirect instruction is permitted
  bool src_trusted = dasics_in_trusted_zone(pc);
  bool dst_trusted = dasics_in_trusted_zone(newpc);
  bool src_freezone = dasics_match_djumpbound(pc, JUMPCFG_V);
  bool dst_freezone = dasics_match_djumpbound(newpc, JUMPCFG_V);

  Logm("[Dasics Redirect] pc: 0x%lx (T:%d F:%d), target:0x%lx (T:%d F:%d)\n", pc, src_trusted, src_freezone, newpc, dst_trusted, dst_freezone);
  Logm("[Dasics Redirect] dretpc: 0x%lx dretmaincall: 0x%lx dretpcfz: 0x%lx\n", dretpc->val, dmaincall->val, dretpcfz->val);

  bool allow_lib_to_main = !src_trusted && dst_trusted && \
    (newpc == dretpc->val || newpc == dmaincall->val);
  bool allow_freezone_to_lib = src_freezone && !dst_trusted && \
    !dst_freezone && (newpc == dretpcfz->val);
  
  bool allow_brjp = src_trusted  || allow_lib_to_main || \
                    dst_freezone || allow_freezone_to_lib;

  if (!allow_brjp) {
    int ex = (cpu.mode == MODE_U) ? EX_DUIAF : EX_DSIAF;
    INTR_TVAL_REG(ex) = newpc;
    // isa_reg_display();
    Logm("Dasics jump exception occur: pc%lx  (st:%d, altm:%d, df:%d, aftl:%d)\n",pc,src_trusted,allow_lib_to_main,dst_freezone,allow_freezone_to_lib);
    longjmp_exception(ex);
  }

  // FIXME: This code is only for UCAS-OS test!
  if (!src_trusted && !src_freezone && !dst_trusted && dst_freezone) {
    //dretpcfz->val = nextpc;
  }
}
#endif  // CONFIG_RV_DASICS

/* raise exception if not trusted */
void dasics_check_trusted(vaddr_t pc) {
  if (!dasics_in_trusted_zone(pc)) {
    int ex = EX_II;
    // isa_reg_display();
    Logm("Dasics illegal instruction: pc%lx\n", pc);
    longjmp_exception(ex);
  }
}

uint8_t pmpcfg_from_index(int idx) {
  // for now, nemu only support 16 pmp entries in a XLEN=64 machine
  int xlen = 64;
  assert(idx < CONFIG_RV_PMP_NUM);
  assert(CONFIG_RV_PMP_NUM <= 16);
  int cfgPerCSR = xlen / 8;
  // no black magic, just get CSR addr from idx
  int cfg_csr_addr;
  switch (idx / cfgPerCSR) {
    case 0: cfg_csr_addr = CSR_PMPCFG0; break;
    case 1: cfg_csr_addr = CSR_PMPCFG2; break;
    // case 2: cfg_csr_addr = CSR_PMPCFG4; break;
    // case 3: cfg_csr_addr = CSR_PMPCFG8; break;
    default: assert(0);
  }
  uint8_t *cfg_reg = (uint8_t *)&csr_array[cfg_csr_addr];
  return *(cfg_reg + (idx % cfgPerCSR));
}

word_t pmpaddr_from_index(int idx) {
  return csr_array[CSR_PMPADDR0 + idx];
}

word_t pmpaddr_from_csrid(int id) {
  return csr_array[id];
}

word_t inline pmp_tor_mask() {
  return -((word_t)1 << (PMP_PLATFORMGARIN - PMP_SHIFT));
}

static inline void update_mstatus_sd() {
  // mstatus.fs is always dirty or off in QEMU 3.1.0
  if (ISDEF(CONFIG_DIFFTEST_REF_QEMU) && mstatus->fs) { mstatus->fs = 3; }
  mstatus->sd = (mstatus->fs == 3);
}

static inline word_t csr_read(word_t *src) {

#ifdef CONFIG_RV_PMP_CSR
  if (is_read_pmpaddr) {
    // If n_pmp is zero, that means pmp is not implemented hence raise trap if it tries to access the csr
    if (CONFIG_RV_PMP_NUM == 0) {
      Loge("pmp number is 0, raise illegal instr exception when read pmpaddr");
      longjmp_exception(EX_II);
      return 0;
    }

    int idx = (src - &csr_array[CSR_PMPADDR0]);
    // Check whether the PMP register is out of bound.
    if (idx >= CONFIG_RV_PMP_NUM) {
      Loge("pmp number is smaller than the index, raise illegal instr exception when read pmpaddr");
      longjmp_exception(EX_II);
      return 0;
    }

    uint8_t cfg = pmpcfg_from_index(idx);
#ifdef CONFIG_SHARE
    if(dynamic_config.debug_difftest) {
      fprintf(stderr, "[NEMU] pmp addr read %d : 0x%016lx\n", idx,
        (cfg & PMP_A) >= PMP_NAPOT ? *src | (~pmp_tor_mask() >> 1) : *src & pmp_tor_mask());
    }
#endif
    if ((cfg & PMP_A) >= PMP_NAPOT)
      return *src | (~pmp_tor_mask() >> 1);
    else
      return *src & pmp_tor_mask();
  }
#endif // CONFIG_RV_PMP_CSR

  if (is_read(mstatus) || is_read(sstatus)) { update_mstatus_sd(); }

  if (is_read(sstatus))     { return mstatus->val & SSTATUS_RMASK; }
#ifdef CONFIG_RVN
  else if (is_read(ustatus)) { return mstatus->val & USTATUS_RMASK; }
#endif  // CONFIG_RVN
  else if (is_read(sie))    { return mie->val & SIE_MASK; }
#ifdef CONFIG_RVN
  else if (is_read(uie))    { return mie->val & UIE_MASK; }
#endif  // CONFIG_RVN
  else if (is_read(mtvec))  { return mtvec->val & ~(0x2UL); }
  else if (is_read(stvec))  { return stvec->val & ~(0x2UL); }
#ifdef CONFIG_RVN
  else if (is_read(utvec))  { return utvec->val & ~(0x2UL); }
#endif  // CONFIG_RVN
  else if (is_read(sip))    { difftest_skip_ref(); return mip->val & SIP_MASK; }
#ifdef CONFIG_RVN
  else if (is_read(uip))    { difftest_skip_ref(); return mip->val & UIP_MASK; }
#endif  // CONFIG_RVN
#ifdef CONFIG_RV_DASICS
  else if (is_read(dsmcfg)) { return dsmcfg->val & DSMCFG_MASK; }
  else if (is_read(dumcfg)) { return dsmcfg->val & DUMCFG_MASK; }
#endif  // CONFIG_RV_DASICS
#ifdef CONFIG_RVV
  else if (is_read(vcsr))   { return (vxrm->val & 0x3) << 1 | (vxsat->val & 0x1); }
#endif
  else if (is_read(fcsr))   {
#ifdef CONFIG_FPU_NONE
    longjmp_exception(EX_II);
#else
    return fcsr->val & FCSR_MASK;
#endif // CONFIG_FPU_NONE
  }
  else if (is_read(fflags)) {
#ifdef CONFIG_FPU_NONE
    longjmp_exception(EX_II);
#else
    return fcsr->fflags.val & FFLAGS_MASK;
#endif // CONFIG_FPU_NONE
  }
  else if (is_read(frm))    {
#ifdef CONFIG_FPU_NONE
    longjmp_exception(EX_II);
#else
    return fcsr->frm & FRM_MASK;
#endif // CONFIG_FPU_NONE
  }
#ifndef CONFIG_SHARE
  else if (is_read(mtime))  { difftest_skip_ref(); return clint_uptime(); }
#endif
  if (is_read(mip)) { difftest_skip_ref(); }

  if (is_read(satp) && cpu.mode == MODE_S && mstatus->tvm == 1) { longjmp_exception(EX_II); }
  return *src;
}

#ifdef CONFIG_RVV
void vcsr_write(uint32_t addr,  rtlreg_t *src) {
  word_t *dest = csr_decode(addr);
  *dest = *src;
}
void vcsr_read(uint32_t addr,  rtlreg_t *dest) {
  word_t *src = csr_decode(addr);
  *dest = *src;
}
#endif // CONFIG_RVV

void disable_time_intr() {
    Log("Disabled machine time interruption\n");
    mie->val = mask_bitset(mie->val, MTIE_MASK, 0);
}

static inline void csr_write(word_t *dest, word_t src) {

  if (is_write(mstatus)) { mstatus->val = mask_bitset(mstatus->val, MSTATUS_WMASK, src); }
  else if (is_write(sstatus)) { mstatus->val = mask_bitset(mstatus->val, SSTATUS_WMASK, src); }
#ifdef CONFIG_RVN
  else if (is_write(ustatus)) { mstatus->val = mask_bitset(mstatus->val, USTATUS_WMASK, src); }
#endif  // CONFIG_RVN
  else if (is_write(sie)) { mie->val = mask_bitset(mie->val, SIE_MASK, src); }
#ifdef CONFIG_RVN
  else if (is_write(uie)) { mie->val = mask_bitset(mie->val, UIE_MASK, src); }
#endif  // CONFIG_RVN
  else if (is_write(mip)) { mip->val = mask_bitset(mip->val, MIP_MASK, src); }
  else if (is_write(sip)) { mip->val = mask_bitset(mip->val, ((cpu.mode == MODE_S) ? SIP_WMASK_S : SIP_MASK), src); }
#ifdef CONFIG_RVN
  else if (is_write(uip)) { mip->val = mask_bitset(mip->val, ((cpu.mode == MODE_U) ? UIP_WMASK_U : UIP_MASK), src); }
#endif  // CONFIG_RVN
  else if (is_write(mtvec)) {
#ifdef XTVEC_VECTORED_MODE
    *dest = src & ~(0x2UL);
#else
    *dest = src & ~(0x3UL);
#endif // XTVEC_VECTORED_MODE
}
  else if (is_write(stvec)) {
#ifdef XTVEC_VECTORED_MODE
    *dest = src & ~(0x2UL);
#else
    *dest = src & ~(0x3UL);
#endif // XTVEC_VECTORED_MODE
}
#ifdef CONFIG_RVN
  else if (is_write(utvec)) {
#ifdef XTVEC_VECTORED_MODE
    *dest = src & ~(0x2UL);
#else
    *dest = src & ~(0x3UL);
#endif // XTVEC_VECTORED_MODE
  }
#endif  // CONFIG_RVN
#ifdef CONFIG_RV_DASICS
  else if (is_write(medeleg)) { *dest = src & 0xff00b3ff; }
#else
  else if (is_write(medeleg)) { *dest = src & 0xb3ff; }
#endif  // CONFIG_RV_DASICS
#ifdef CONFIG_RVN
  else if (is_write(mideleg)) { *dest = src & 0x333; }
  else if (is_write(sedeleg)) { *dest = src & medeleg->val; }
  else if (is_write(sideleg)) { *dest = src & mideleg->val; }
#else
  else if (is_write(mideleg)) { *dest = src & 0x222; }
#endif  // CONFIG_RVN
#ifdef CONFIG_RVV
  else if (is_write(vcsr)) { vxrm->val = (src >> 1) & 0x3; vxsat->val = src & 0x1; }
#endif
#ifdef CONFIG_MISA_UNCHANGEABLE
  else if (is_write(misa)) { /* do nothing */ }
#endif
  else if (is_write(mepc)) { *dest = src & (~0x1UL); }
  else if (is_write(sepc)) { *dest = src & (~0x1UL); }
#ifdef CONFIG_RVN
  else if (is_write(uepc)) { *dest = src & (~0x1UL); }
#endif  // CONFIG_RVN
  else if (is_write(fflags)) {
#ifdef CONFIG_FPU_NONE
  longjmp_exception(EX_II);
#else
    *dest = src & FFLAGS_MASK;
    fcsr->val = (frm->val)<<5 | fflags->val;
    // fcsr->fflags.val = src;
#endif // CONFIG_FPU_NONE
  }
  else if (is_write(frm)) {
#ifdef CONFIG_FPU_NONE
  longjmp_exception(EX_II);
#else
    *dest = src & FRM_MASK;
    fcsr->val = (frm->val)<<5 | fflags->val;
    // fcsr->frm = src;
#endif // CONFIG_FPU_NONE
  }
  else if (is_write(fcsr)) {
#ifdef CONFIG_FPU_NONE
  longjmp_exception(EX_II);
#else
    *dest = src & FCSR_MASK;
    fflags->val = src & FFLAGS_MASK;
    frm->val = ((src)>>5) & FRM_MASK;
    // *dest = src & FCSR_MASK;
#endif // CONFIG_FPU_NONE
  }
#ifdef CONFIG_RV_PMP_CSR
  else if (is_write_pmpaddr) {
    Logtr("Writing pmp addr");
    // If no PMPs are configured, disallow access to all.  Otherwise, allow
    // access to all, but unimplemented ones are hardwired to zero.
    if (CONFIG_RV_PMP_NUM == 0)
      return;

    Logtr("PMP updated\n");
    int idx = dest - &csr_array[CSR_PMPADDR0];
    // Check whether the PMP register is out of bound.
    if (idx >= CONFIG_RV_PMP_NUM) {
      Loge("pmp number is smaller than the index, raise illegal instr exception when read pmpaddr");
      longjmp_exception(EX_II);
      return;
    }

    word_t cfg = pmpcfg_from_index(idx);
    bool locked = cfg & PMP_L;
    // Note that the last pmp cfg do not have next_locked or next_tor
    bool next_locked = idx == (CONFIG_RV_PMP_NUM-1) ? false : idx < CONFIG_RV_PMP_NUM && (pmpcfg_from_index(idx+1) & PMP_L);
    bool next_tor = idx == (CONFIG_RV_PMP_NUM-1) ? false : idx < CONFIG_RV_PMP_NUM && (pmpcfg_from_index(idx+1) & PMP_A) == PMP_TOR;
    if (idx < CONFIG_RV_PMP_NUM && !locked && !(next_locked && next_tor)) {
      *dest = src & (((word_t)1 << (CONFIG_PADDRBITS - PMP_SHIFT)) - 1);
    }
#ifdef CONFIG_SHARE
    if(dynamic_config.debug_difftest) {
      fprintf(stderr, "[NEMU] write pmp addr%d to %016lx\n",idx, *dest);
    }
#endif

    mmu_tlb_flush(0);
  }
  else if (is_write_pmpcfg) {
    // Log("Writing pmp config");
    if (CONFIG_RV_PMP_NUM == 0)
      return;

    int xlen = 64;
    word_t cfg_data = 0;
    for (int i = 0; i < xlen / 8; i ++ ) {
      word_t cfg = ((src >> (i*8)) & 0xff) & (PMP_R | PMP_W | PMP_X | PMP_A | PMP_L);
      cfg &= ~PMP_W | ((cfg & PMP_R) ? PMP_W : 0); // Disallow R=0 W=1
      if (PMP_PLATFORMGARIN != PMP_SHIFT && (cfg & PMP_A) == PMP_NA4)
        cfg |= PMP_NAPOT; // Disallow A=NA4 when granularity > 4
      cfg_data |= (cfg << (i*8));
    }
#ifdef CONFIG_SHARE
    if(dynamic_config.debug_difftest) {
      int idx = dest - &csr_array[CSR_PMPCFG0];
      fprintf(stderr, "[NEMU] write pmp cfg%d to %016lx\n",idx, cfg_data);
    }
#endif

    *dest = cfg_data;

    mmu_tlb_flush(0);
  }
#endif
#ifdef CONFIG_RV_DASICS
  else if (is_write(dsmcfg) || is_write(dumcfg)) {
    // Perform CLS logic of dmcfg first
    bool val_scls = (src & MCFG_SCLS) != 0;
    bool val_ucls = (src & MCFG_UCLS) != 0;

    if (val_scls) {
      dumbound0->val = 0;
      dumbound1->val = 0;
    }

    if (val_scls || val_ucls) {
      for (int i = 0; i < (MAX_DASICS_LIBBOUNDS >> 3); ++i) {
        csr_array[CSR_DLCFG0 + i] = 0;
      }
      for (int i = 0; i < MAX_DASICS_LIBBOUNDS; ++i) {
        csr_array[CSR_DLBOUND0 + (i << 1)] = 0;
        csr_array[CSR_DLBOUND1 + (i << 1)] = 0;
      }

      dmaincall->val = 0;
      dretpc->val = 0;
      dretpcfz->val = 0;
    }

    // Then update dmcfg itself. CLS bit has already taken effect, thus set it to zero
    word_t mask = is_write(dsmcfg) ? DSMCFG_MASK : DUMCFG_MASK;
    dsmcfg->val = (dsmcfg->val & ~mask) | (src & ~MCFG_SCLS & ~MCFG_UCLS & mask);
  } else if (is_write_dasics_mem_bound || is_write_dasics_jump_bound) {
    *dest = src & ~BOUND_ADDR_ALGIN; 
    if(is_write_dasics_jump_bound) Logm("[write jump bound]: write addr %016lx src: %lx\n",*dest,src );
  }
#endif  // CONFIG_RV_DASICS
  else if (is_write(satp)) {
    if (cpu.mode == MODE_S && mstatus->tvm == 1) {
      longjmp_exception(EX_II);
    }
    // Only support Sv39, ignore write that sets other mode
    if ((src & SATP_SV39_MASK) >> 60 == 8 || (src & SATP_SV39_MASK) >> 60 == 0)
      *dest = MASKED_SATP(src);
  } else { *dest = src; }

  bool need_update_mstatus_sd = false;
  if (is_write(fflags) || is_write(frm) || is_write(fcsr)) {
#ifdef CONFIG_FPU_NONE
  longjmp_exception(EX_II);
#else
    fp_set_dirty();
    fp_update_rm_cache(fcsr->frm);
    need_update_mstatus_sd = true;
#endif // CONFIG_FPU_NONE

  }

  if (is_write(sstatus) || is_write(mstatus) || need_update_mstatus_sd) {
    update_mstatus_sd();
  }

  if (is_write(mstatus) || is_write(satp)) { update_mmu_state(); }
  if (is_write(satp)) { mmu_tlb_flush(0); } // when satp is changed(asid | ppn), flush tlb.
  if (is_write(mstatus) || is_write(sstatus) || is_write(satp) ||
      is_write(mie) || is_write(sie) || is_write(mip) || is_write(sip)) {
    set_sys_state_flag(SYS_STATE_UPDATE);
  }
#ifdef CONFIG_RVN
  if (is_write(ustatus) || is_write(uie) || is_write(uip)) {
    set_sys_state_flag(SYS_STATE_UPDATE);
  }
#endif  // CONFIG_RVN
#ifdef CONFIG_RVV
  if (is_write(vcsr) || is_write(vstart) || is_write(vxsat) || is_write(vxrm)) {
    //vp_set_dirty();
  }
#endif
}

word_t csrid_read(uint32_t csrid) {
  return csr_read(csr_decode(csrid));
}

static void csrrw(rtlreg_t *dest, const rtlreg_t *src, uint32_t csrid, vaddr_t pc) {
  if (!csr_is_legal(csrid, src != NULL, pc)) {
    Logti("Illegal csr id %u", csrid);
    longjmp_exception(EX_II);
    return;
  }
  word_t *csr = csr_decode(csrid);
  // Log("Decoding csr id %u to %p", csrid, csr);
  word_t tmp = (src != NULL ? *src : 0);
  if (dest != NULL) { *dest = csr_read(csr); }
  if (src != NULL) { csr_write(csr, tmp); }
}

static word_t priv_instr(uint32_t op, const rtlreg_t *src) {
  switch (op) {
#ifndef CONFIG_MODE_USER
#ifdef CONFIG_RVN
    case 0x002: // uret
      mstatus->uie = mstatus->upie;
      mstatus->upie = (ISDEF(CONFIG_DIFFTEST_REF_QEMU) ? 0 // this is bug of QEMU
          : 1);
      cpu.mode = MODE_U;
      mstatus->mprv = 0;
      update_mmu_state();
      return uepc->val;
#endif  // CONFIG_RVN
    case 0x102: // sret
      if ((cpu.mode == MODE_S && mstatus->tsr) || cpu.mode < MODE_S) {
        longjmp_exception(EX_II);
      }
      mstatus->sie = mstatus->spie;
      mstatus->spie = (ISDEF(CONFIG_DIFFTEST_REF_QEMU) ? 0 // this is bug of QEMU
          : 1);
      cpu.mode = mstatus->spp;
      if (mstatus->spp != MODE_M) { mstatus->mprv = 0; }
      mstatus->spp = MODE_U;
      update_mmu_state();
      return sepc->val;
    case 0x302: // mret
      if (cpu.mode < MODE_M) {
        longjmp_exception(EX_II);
      }
      mstatus->mie = mstatus->mpie;
      mstatus->mpie = (ISDEF(CONFIG_DIFFTEST_REF_QEMU) ? 0 // this is bug of QEMU
          : 1);
      cpu.mode = mstatus->mpp;
      if (mstatus->mpp != MODE_M) { mstatus->mprv = 0; }
      mstatus->mpp = MODE_U;
      update_mmu_state();
      Loge("Executing mret to 0x%lx", mepc->val);
      return mepc->val;
      break;
#ifdef CONFIG_RV_SVINVAL
    case 0x180: // sfence.w.inval
      if (!srnctl->svinval) {
        longjmp_exception(EX_II);
      }
      break;
    case 0x181: // sfence.inval.ir
      if (!srnctl->svinval) {
        longjmp_exception(EX_II);
      }
      break;
#endif // CONFIG_RV_SVINVAL
    case 0x105: // wfi
      if (cpu.mode < MODE_M && mstatus->tw == 1){
        longjmp_exception(EX_II);
      }
    break;
#endif // CONFIG_MODE_USER
    case -1: // fence.i
      set_sys_state_flag(SYS_STATE_FLUSH_TCACHE);
      break;
    default:
      switch (op >> 5) { // instr[31:25]
        case 0x09: // sfence.vma
          // Described in 3.1.6.5 Virtualization Support in mstatus Register
          // When TVM=1, attempts to read or write the satp CSR or execute an SFENCE.VMA or SINVAL.VMA instruction
          // while executing in S-mode will raise an illegal instruction exception.
          if (cpu.mode == MODE_S && mstatus->tvm == 1)
            longjmp_exception(EX_II);
          mmu_tlb_flush(*src);
          break;
#ifdef CONFIG_RV_SVINVAL
        case 0x0b: // sinval.vma
          if ((cpu.mode == MODE_S && mstatus->tvm == 1) ||
            !srnctl->svinval) { // srnctl contrl extension enable or not
            longjmp_exception(EX_II);
          }
          mmu_tlb_flush(*src);
          break;
#endif // CONFIG_RV_SVINVAL
        default:
#ifdef CONFIG_SHARE
          longjmp_exception(EX_II);
#else
          panic("Unsupported privilege operation = %d", op);
#endif // CONFIG_SHARE
      }
  }
  return 0;
}

void isa_hostcall(uint32_t id, rtlreg_t *dest, const rtlreg_t *src1,
    const rtlreg_t *src2, word_t imm, vaddr_t pc) {
  word_t ret = 0;
  switch (id) {
    case HOSTCALL_CSR: csrrw(dest, src1, imm, pc); return;
#ifdef CONFIG_MODE_USER
    case HOSTCALL_TRAP:
      Assert(imm == 0x8, "Unsupported exception = %ld", imm);
      uintptr_t host_syscall(uintptr_t id, uintptr_t arg1, uintptr_t arg2,
          uintptr_t arg3, uintptr_t arg4, uintptr_t arg5, uintptr_t arg6);
      cpu.gpr[10]._64 = host_syscall(cpu.gpr[17]._64, cpu.gpr[10]._64, cpu.gpr[11]._64,
          cpu.gpr[12]._64, cpu.gpr[13]._64, cpu.gpr[14]._64, cpu.gpr[15]._64);
      ret = *src1 + 4;
      break;
#else
    case HOSTCALL_TRAP: 
#ifdef CONFIG_RV_DASICS
      bool hostcall_trusted = dasics_in_trusted_zone(pc);

      if (!hostcall_trusted && cpu.mode == MODE_U) {
        ret = raise_intr(EX_DUEF, *src1);
      }
      else if (!hostcall_trusted && cpu.mode == MODE_S) {
        ret = raise_intr(EX_DSEF, *src1);
      }
      else {
        ret = raise_intr(imm, *src1);
      }
      break;
#else
      ret = raise_intr(imm, *src1); break;
#endif  // CONFIG_RV_DASICS
#endif
    case HOSTCALL_PRIV: ret = priv_instr(imm, src1); break;
    default: panic("Unsupported hostcall ID = %d", id);
  }
  if (dest) *dest = ret;
}
