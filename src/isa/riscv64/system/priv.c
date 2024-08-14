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
#include "../local-include/trigger.h"
#include <cpu/cpu.h>
#include <cpu/difftest.h>
#include <memory/paddr.h>
#include <stdlib.h>

int update_mmu_state();
uint64_t clint_uptime();
void fp_set_dirty();
void fp_update_rm_cache(uint32_t rm);
void vp_set_dirty();

uint64_t get_abs_instr_count();

rtlreg_t csr_array[4096] = {};

#define CSRS_DEF(name, addr) \
  concat(name, _t)* const name = (concat(name, _t) *)&csr_array[addr];

MAP(CSRS, CSRS_DEF)

#define CSRS_EXIST(name, addr) csr_exist[addr] = 1;
static bool csr_exist[4096] = {};
void init_csr() {
  MAP(CSRS, CSRS_EXIST)
  #ifdef CONFIG_RVH
  cpu.v = 0;
  #endif
};

#ifdef CONFIG_RV_SDTRIG
void init_trigger() {
  cpu.TM = (TriggerModule*) malloc(sizeof (TriggerModule));
  for (int i = 0; i < CONFIG_TRIGGER_NUM; i++){
    cpu.TM->triggers[i].tdata1.val = 0;
    cpu.TM->triggers[i].tdata1.common.type = TRIG_TYPE_DISABLE;
  }
  tselect->val = 0;
  tdata1->val = cpu.TM->triggers[tselect->val].tdata1.val;
  tinfo->val = (1 << TRIG_TYPE_MCONTROL);
  tcontrol->val = 0;
}
#endif // CONFIG_RV_SDTRIG

// check s/h/mcounteren for counters, throw exception if counter is not enabled.
// also check h/mcounteren h/menvcfg for sstc
static inline void csr_counter_enable_check(uint32_t addr) {
  int count_bit = 1 << (addr - 0xC00);
  bool is_sstc_csr = MUXDEF(CONFIG_RV_SSTC, (addr == 0x14D) || (addr == 0x24D), 0);

  if (is_sstc_csr) {
      count_bit = 1 << 1; // counteren.TM
  }

  // priv-mode & counter-enable -> exception-type
  // | MODE         | VU    | VS    | U     | S/HS  | M     |
  // | ~mcounteren  | EX_II | EX_II | EX_II | EX_II | OK    | (counters & s/vstimecmp)
  // | ~menvccfg    | EX_II | EX_II | EX_II | EX_II | OK    | (s/vstimecmp)
  // | ~hcounteren  | EX_VI | EX_VI | OK    | OK    | OK    | (counters & stimecmp)
  // | ~scounteren  | EX_VI | OK    | EX_II | OK    | OK    | (counters)
  if (cpu.mode < MODE_M && (!(count_bit & mcounteren->val) || (is_sstc_csr && !menvcfg->stce))) {
    Logti("Illegal CSR accessing (0x%X): the bit in mcounteren is not set", addr);
    longjmp_exception(EX_II);
  }

  #ifdef CONFIG_RVH
    if (cpu.v && (!(count_bit & hcounteren->val) || (is_sstc_csr && !henvcfg->stce))) {
      Logti("Illegal CSR accessing (0x%X): the bit in hcounteren is not set", addr);
      longjmp_exception(EX_VI);
    }
  #endif // CONFIG_RVH

  if (cpu.mode < MODE_S && !(count_bit & scounteren->val) && !is_sstc_csr) {
    Logti("Illegal CSR accessing (0x%X): the bit in scounteren is not set", addr);
    #ifdef CONFIG_RVH
      if (cpu.v) {
        longjmp_exception(EX_VI);
      }
    #endif // CONFIG_RVH
    longjmp_exception(EX_II);
  }
}

static inline bool csr_is_legal(uint32_t addr, bool need_write) {
  assert(addr < 4096);
  // Attempts to access a non-existent CSR raise an illegal instruction exception.
  if(!csr_exist[addr]) {
#ifdef CONFIG_PANIC_ON_UNIMP_CSR
    panic("[NEMU] unimplemented CSR 0x%x", addr);
#endif
    return false;
  }
#ifdef CONFIG_RV_SDTRIG
  bool isDebugReg = ((addr >> 4) & 0xff) == 0x7b; // addr(11,4)
  if(isDebugReg)
    return false;
#endif
  // Attempts to access a CSR without appropriate privilege level
  int lowest_access_priv_level = (addr & 0b11 << 8) >> 8; // addr(9,8)
#ifdef CONFIG_RVH
  int priv = cpu.mode == MODE_S ? MODE_HS : cpu.mode;
  if(priv < lowest_access_priv_level){
    if(cpu.v && lowest_access_priv_level <= MODE_HS)
      longjmp_exception(EX_VI);
    return false;
  }
#else
  if (!(cpu.mode >= lowest_access_priv_level)) {
    return false;
  }
#endif
  // or to write a read-only register also raise illegal instruction exceptions.
  if (need_write && (addr >> 10) == 0x3) {
    return false;
  }
  // Attempts to access unprivileged counters without s/h/mcounteren
  if ((addr >= 0xC00 && addr <= 0xC1F) || (addr == 0x14D) || (addr == 0x24D)) {
    csr_counter_enable_check(addr);
  }

  return true;
}

static inline word_t* csr_decode(uint32_t addr) {
  assert(addr < 4096);
  // Now we check if CSR is implemented / legal to access in csr_is_legal()
  // Assert(csr_exist[addr], "unimplemented CSR 0x%x at pc = " FMT_WORD, addr, cpu.pc);

  return &csr_array[addr];
}

// WPRI, SXL, UXL cannot be written

// base mstatus wmask
#define MSTATUS_WMASK_BASE (0x7e19aaUL)

// FS
#if !defined(CONFIG_FPU_NONE) || defined(CONFIG_RV_MSTATUS_FS_WRITABLE)
#define MSTATUS_WMASK_FS (0x3UL << 13)
#else
#define MSTATUS_WMASK_FS 0x0
#endif

// rvh fields of mstatus
#if defined(CONFIG_RVH)
#define MSTATUS_WMASK_RVH (3UL << 38)
#else
#define MSTATUS_WMASK_RVH 0
#endif

// rvv fields of mstatus
#if defined(CONFIG_RVV)
#define MSTATUS_WMASK_RVV (3UL << 9)
#else
#define MSTATUS_WMASK_RVV 0
#endif

// final mstatus wmask: dependent of the ISA extensions
#define MSTATUS_WMASK (MSTATUS_WMASK_BASE | MSTATUS_WMASK_FS | MSTATUS_WMASK_RVH | MSTATUS_WMASK_RVV)

// wmask of sstatus is given by masking the valid fields in sstatus
#define SSTATUS_WMASK (MSTATUS_WMASK & SSTATUS_RMASK)

// hstatus wmask
#if defined(CONFIG_RVH)
#define HSTATUS_WMASK ((1 << 22) | (1 << 21) | (1 << 20) | (1 << 18) | (0x3f << 12) | (1 << 9) | (1 << 8) | (1 << 7) | (1 << 6) | (1 << 5))
#else
#define HSTATUS_WMASK 0
#endif

#ifdef CONFIG_RV_ZICNTR
  #define COUNTEREN_ZICNTR_MASK (0x7UL)
#else // CONFIG_RV_ZICNTR
  #define COUNTEREN_ZICNTR_MASK (0x0)
#endif // CONFIG_RV_ZICNTR

#ifdef CONFIG_RV_ZIHPM
  #define COUNTEREN_ZIHPM_MASK (0xfffffff8UL)
#else // CONFIG_RV_ZIHPM
  #define COUNTEREN_ZIHPM_MASK (0x0)
#endif // CONFIG_RV_ZIHPM

#define COUNTEREN_MASK (COUNTEREN_ZICNTR_MASK | COUNTEREN_ZIHPM_MASK)


#ifdef CONFIG_RV_CSR_MCOUNTINHIBIT_CNTR
  #define MCOUNTINHIBIT_CNTR_MASK (0x5UL)
#else // CONFIG_RV_CSR_MCOUNTINHIBIT_CNTR
  #define MCOUNTINHIBIT_CNTR_MASK (0x0)
#endif // CONFIG_RV_CSR_MCOUNTINHIBIT_CNTR

#ifdef CONFIG_RV_CSR_MCOUNTINHIBIT_HPM
  #define MCOUNTINHIBIT_HPM_MASK (0xFFFFFFF8UL)
#else // CONFIG_RV_CSR_MCOUNTINHIBIT_HPM
  #define MCOUNTINHIBIT_HPM_MASK (0x0)
#endif // CONFIG_RV_CSR_MCOUNTINHIBIT_CNTR

#define MCOUNTINHIBIT_MASK (MCOUNTINHIBIT_CNTR_MASK | MCOUNTINHIBIT_HPM_MASK)

#ifdef CONFIG_RVH
#define MIDELEG_FORCED_MASK HSI_MASK // mideleg bits 2、6、10、12 are read_only one
#define HVIP_MASK     VSI_MASK       // ((1 << 10) | (1 << 6) | (1 << 2))
#define HIP_RMASK     HSI_MASK
#define HIP_WMASK     MIP_VSSIP
#define HIE_RMASK     HSI_MASK
#define HIE_WMASK     HSI_MASK
#define HIDELEG_MASK  (VSI_MASK | MUXDEF(CONFIG_RV_SHLCOFIDELEG, MIP_LCOFIP, 0))
#define HEDELEG_MASK  0xb1ff
#endif

#define MEDELEG_MASK MUXDEF(CONFIG_RVH, MUXDEF(CONFIG_RV_SDTRIG, 0xf0b7f7, 0xf0b7ff), MUXDEF(CONFIG_RV_SDTRIG, 0xb3f7, 0xb3ff))

#define MIDELEG_WMASK_BASE 0x222
#define MIDELEG_WMASK MUXDEF(CONFIG_RV_SSCOFPMF, (MIDELEG_WMASK_BASE | 1 << IRQ_LCOFI), MIDELEG_WMASK_BASE)

#define MIE_MASK_BASE 0xaaa
#define MIP_MASK_BASE ((1 << 9) | (1 << 5) | (1 << 1))
#ifdef CONFIG_RVH
#define MIE_MASK_H ((1 << 2) | (1 << 6) | (1 << 10) | (1 << 12))
#define MIP_MASK_H MIP_VSSIP
#else
#define MIE_MASK_H 0
#define MIP_MASK_H 0
#endif // CONFIG_RVH

#define SIE_MASK_BASE (0x222 & mideleg->val)
#define SIP_MASK (0x222 & mideleg->val)
#define SIP_WMASK_S 0x2
#define MTIE_MASK (1 << 7)

#define LCOFI MUXDEF(CONFIG_RV_SSCOFPMF, (1 << 13), 0)
#define LCI MUXDEF(CONFIG_RV_AIA, LCI_MASK, 0)

// sie
#define SIE_LCOFI_MASK_MIE (mideleg->val & LCOFI)

// mvien
#define MVIEN_MASK (LCI | LCOFI | (1 << 9) | (1 << 1))
// hvien
#define HVIEN_MSAK (LCI | LCOFI)

#define FFLAGS_MASK 0x1f
#define FRM_MASK 0x07
#define FCSR_MASK 0xff
#define SATP_SV39_MASK 0xf000000000000000ULL

#define SCOUNTOVF_WMASK 0xfffffff8ULL

// Smcsrind/Sscsrind is not implemented. bit 60(CSRIND) read-only 1.
#define STATEEN0_CSRIND  0x1000000000000000ULL
#define MSTATEEN0_WMASK  0xdc00000000000001ULL
#define HSTATEEN0_WMASK  0xdc00000000000001ULL
#define SSTATEEN0_WMASK  0x0000000000000001ULL // 32 bits

#define is_read(csr) (src == (void *)(csr))
#define is_write(csr) (dest == (void *)(csr))
#define is_access(csr) (dest_access == (void *)(csr))
#define mask_bitset(old, mask, new) (((old) & ~(mask)) | ((new) & (mask)))

#define is_pmpcfg(p) (p >= &(csr_array[CSR_PMPCFG_BASE]) && p < &(csr_array[CSR_PMPCFG_BASE + CSR_PMPCFG_MAX_NUM]))
#define is_pmpaddr(p) (p >= &(csr_array[CSR_PMPADDR_BASE]) && p < &(csr_array[CSR_PMPADDR_BASE + CSR_PMPADDR_MAX_NUM]))
#define is_hpmcounter(p) (p >= &(csr_array[CSR_HPMCOUNTER_BASE]) && p < &(csr_array[CSR_HPMCOUNTER_BASE + CSR_HPMCOUNTER_NUM]))
#define is_mhpmcounter(p) (p >= &(csr_array[CSR_MHPMCOUNTER_BASE]) && p < &(csr_array[CSR_MHPMCOUNTER_BASE + CSR_MHPMCOUNTER_NUM]))
#define is_mhpmevent(p) (p >= &(csr_array[CSR_MHPMEVENT_BASE]) && p < &(csr_array[CSR_MHPMEVENT_BASE + CSR_MHPMEVENT_NUM]))

// get 8-bit config of one PMP entries by index.
uint8_t pmpcfg_from_index(int idx) {
  // Nemu support up to 64 pmp entries in a XLEN=64 machine.
  int xlen = 64;
  // Configuration register of one entry is 8-bit.
  int bits_per_cfg = 8;
  // For RV64, one pmpcfg CSR contains configuration of 8 entries (64 / 8 = 8).
  int cfgs_per_csr = xlen / bits_per_cfg;
  // For RV64, only 8 even-numbered pmpcfg CSRs hold the configuration.
  int pmpcfg_csr_addr = CSR_PMPCFG_BASE + idx / cfgs_per_csr * 2;

  uint8_t *cfg_reg = (uint8_t *)&csr_array[pmpcfg_csr_addr];
  return *(cfg_reg + (idx % cfgs_per_csr));
}

word_t pmpaddr_from_index(int idx) {
  return csr_array[CSR_PMPADDR_BASE + idx];
}

word_t inline pmp_tor_mask() {
  return -((word_t)1 << (CONFIG_PMP_GRANULARITY - PMP_SHIFT));
}

#ifndef CONFIG_FPU_NONE
static inline bool require_fs() {
  if ((mstatus->val & MSTATUS_WMASK_FS) != 0) {
  #ifdef CONFIG_RVH
    if (!cpu.v || (vsstatus->val & MSTATUS_WMASK_FS) != 0) {
      return true;
    }
    return false;
  #endif // CONFIG_RVH
    return true;
  }
  return false;
}
#endif // CONFIG_FPU_NONE

#ifdef CONFIG_RVV
static inline bool require_vs() {
  if ((mstatus->val & MSTATUS_WMASK_RVV) != 0) {
  #ifdef CONFIG_RVH
    if (!cpu.v || (vsstatus->val & MSTATUS_WMASK_RVV) != 0) {
      return true;
    }
    return false;
  #endif // CONFIG_RVH
    return true;
  }
  return false;
}
#endif // CONFIG_RVV

inline word_t gen_status_sd(word_t status) {
  mstatus_t xstatus = (mstatus_t)status;
  bool fs_dirty = xstatus.fs == EXT_CONTEXT_DIRTY;
  bool vs_dirty = xstatus.vs == EXT_CONTEXT_DIRTY;
  return ((word_t)(fs_dirty || vs_dirty)) << 63;
}

static inline word_t get_mcycle() {
  #ifdef CONFIG_RV_CSR_MCOUNTINHIBIT_CNTR
    if (mcountinhibit->val & 0x1) {
      return mcycle->val;
    }
  #endif // CONFIG_RV_CSR_MCOUNTINHIBIT_CNTR
  return mcycle->val + get_abs_instr_count();
}

static inline word_t get_minstret() {
  #ifdef CONFIG_RV_CSR_MCOUNTINHIBIT_CNTR
    if (mcountinhibit->val & 0x4) {
      return minstret->val;
    }
  #endif // CONFIG_RV_CSR_MCOUNTINHIBIT_CNTR
  return minstret->val + get_abs_instr_count();
}

static inline word_t set_mcycle(word_t src) {
  #ifdef CONFIG_RV_CSR_MCOUNTINHIBIT_CNTR
    if (mcountinhibit->val & 0x1) {
      return src;
    }
  #endif // CONFIG_RV_CSR_MCOUNTINHIBIT_CNTR
  return src - get_abs_instr_count();
}

static inline word_t set_minstret(word_t src) {
  #ifdef CONFIG_RV_CSR_MCOUNTINHIBIT_CNTR
    if (mcountinhibit->val & 0x4) {
      return src;
    }
  #endif // CONFIG_RV_CSR_MCOUNTINHIBIT_CNTR
  return src - get_abs_instr_count();
}

#ifdef CONFIG_RV_AIA
static inline word_t get_ie(word_t old_value, word_t begin, word_t end, bool v_sie) {
  if (v_sie) {
    for (int i=begin; i<=end; i++) {
      if (((hideleg->val & (1 << i)) >> i) & ((mideleg->val & (1 << i)) >> i)) {
        old_value |= mie->val & (1 << i);
      } else if (((hideleg->val & (1 << i)) >> i) & ~((mideleg->val & (1 << i)) >> i) & ((mvien->val & (1 << i)) >> i)) {
        old_value |= sie->val & (1 << i);
      } else if (~((hideleg->val & (1 << i)) >> i) & ((hvien->val & (1 << i)) >> i)) {
        old_value |= vsie->val & (1 << i);
      } else {
        old_value |= 0;
      }
    }
  } else {
    for (int i=begin; i<=end; i++) {
      if ((mideleg->val & (1 << i)) >> i) {
        old_value |= mie->val & (1 << i);
      } else if (~((mideleg->val & (1 << i)) >> i) & ((mvien->val & (1 << i)) >> i)) {
        old_value |= sie->val & (1 << i);
      }
    }
  }
  return old_value;
}
#endif

#ifdef CONFIG_RV_AIA
static inline void set_ie(word_t src, word_t begin, word_t end, bool v_sie) {
  if (v_sie) {
    for (int i=begin; i<=end; i++) {
      if (((hideleg->val & (1 << i)) >> i) & ((mideleg->val & (1 << i)) >> i)) {
        mie->val = mask_bitset(mie->val, (1 << i), src);
      } else if (((hideleg->val & (1 << i)) >> i) & ~((mideleg->val & (1 << i)) >> i) & ((mvien->val & (1 << i)) >> i)) {
        sie->val = mask_bitset(sie->val, (1 << i), src);
      } else if (~((hideleg->val & (1 << i)) >> i) & ((hvien->val & (1 << i)) >> i)) {
        vsie->val = mask_bitset(vsie->val, (1 << i), src);
      }
    }
  } else {
    for (int i=begin; i<=end; i++) {
      if ((mideleg->val & (1 << i)) >> i) {
        mie->val = mask_bitset(mie->val, (1 << i), src);
      } else if (~((mideleg->val & (1 << i)) >> i) & ((mvien->val & (1 << i)) >> i)) {
        sie->val = mask_bitset(sie->val, (1 << i), src);
      }
    }
  }
}
#endif

static inline word_t get_sie() {
  word_t tmp = 0;
#ifdef CONFIG_RV_AIA
  tmp |= get_ie(tmp, 1, 1, false);
  if (mideleg->sti) {
    tmp |= mie->stie << 5;
  } else {
    tmp |= 0;
  }
  tmp |= get_ie(tmp, 9, 9, false);
  tmp |= get_ie(tmp, 13, 63, false);
#ifdef CONFIG_RV_SSCOFPMF
  tmp |= get_ie(tmp, 13, 13, false);
#endif
#else
  tmp = mie->val & SIE_MASK_BASE;
#ifdef CONFIG_RV_SSCOFPMF
  if (mideleg->lcofi) {
    tmp |= mie->lcofie << 13;
  } else {
    tmp |= 0;
  }
#endif
#endif
  return tmp;
}

static inline void set_sie(word_t src) {
#ifdef CONFIG_RV_AIA
  set_ie(src, 1, 1, false);

  if (mideleg->sti) {
    mie->stie = (src & (1 << 5)) >> 5;
  }

  set_ie(src, 9, 9, false);
  set_ie(src, 13, 63, false);
#ifdef CONFIG_RV_SSCOFPMF
  set_ie(src, 13, 13, false);
#endif
#else
#ifdef CONFIG_RV_SSCOFPMF
  mie->val = mask_bitset(mie->val, (SIE_MASK_BASE | SIE_LCOFI_MASK_MIE), src);
#else
  mie->val = mask_bitset(mie->val, SIE_MASK_BASE, src);
#endif
#endif
}

static inline void set_tvec(word_t* dest, word_t src) {
  tvec_t newVal = (tvec_t)src;
  tvec_t* destPtr = (tvec_t*)dest;
#ifdef CONFIG_XTVEC_VECTORED_MODE
  if (newVal.mode < 2) {
    destPtr->mode = newVal.mode;
  }
#else
  destPtr->mode = 0; // only DIRECT mode is supported
#endif // CONFIG_XTVEC_VECTORED_MODE
  destPtr->base = newVal.base;
}

#ifdef CONFIG_RVH
static inline word_t get_v_sie() {
  word_t tmp = 0;
#ifdef CONFIG_RV_AIA
  tmp = (mie->val & VSI_MASK) >> 1;
  tmp |= get_ie(tmp, 13, 63, true);
#ifdef CONFIG_RV_SSCOFPMF
  tmp |= get_ie(tmp, 13, 13, true);
#endif
#else
  tmp = (mie->val & VSI_MASK) >> 1;
#ifdef CONFIG_RV_SSCOFPMF
  if (mideleg->lcofi & hideleg->lcofi) {
    tmp |= mie->lcofie << 13;
  } else {
    tmp |= 0;
  }
#endif
#endif
  return tmp;
}
#endif

#ifdef CONFIG_RVH
static inline void set_v_sie(word_t src) {
#ifdef CONFIG_RV_AIA
  mie->val = mask_bitset(mie->val, VSI_MASK, src << 1);
  set_ie(src, 13, 63, true);
#else
  mie->val = mask_bitset(mie->val, VSI_MASK, src << 1);
#endif
#ifdef CONFIG_RV_SSCOFPMF
  if (mideleg->lcofi & hideleg->lcofi) {
    mie->val = mask_bitset(mie->val, LCOFI, src);
  }
#endif
}
#endif

#ifdef CONFIG_RVH
static inline word_t get_vsie() {
  word_t tmp;
  tmp = (mie->val & (hideleg->val & (mideleg->val | MIDELEG_FORCED_MASK)) & VSI_MASK) >> 1;
#ifdef CONFIG_RV_AIA
  tmp |= get_ie(tmp, 13, 63, true);
#ifdef CONFIG_RV_SSCOFPMF
  tmp |= get_ie(tmp, 13, 13, true);
#endif
#endif
  return tmp;
}
#endif

#ifdef CONFIG_RVH
static inline void set_vsie(word_t src) {
  mie->val = mask_bitset(mie->val, VSI_MASK & (hideleg->val & (mideleg->val | MIDELEG_FORCED_MASK)), src << 1);
#ifdef CONFIG_RV_AIA
  set_ie(src, 13, 63, true);
#ifdef CONFIG_RV_SSCOFPMF
  set_ie(src, 13, 13, true);
#endif
#endif
}
#endif

static inline void update_counter_mcountinhibit(word_t old, word_t new) {
  #ifdef CONFIG_RV_CSR_MCOUNTINHIBIT_CNTR
    bool old_cy = old & 0x1;
    bool old_ir = old & 0x4;
    bool new_cy = new & 0x1;
    bool new_ir = new & 0x4;

    if (old_cy && !new_cy) { // CY: 1 -> 0
      mcycle->val = mcycle->val - get_abs_instr_count();
    }
    if (!old_cy && new_cy) { // CY: 0 -> 1
      mcycle->val = mcycle->val + get_abs_instr_count();
    }
    if (old_ir && !new_ir) { // IR: 1 -> 0
      minstret->val = minstret->val - get_abs_instr_count();
    }
    if (!old_ir && new_ir) { // IR: 0 -> 1
      minstret->val = minstret->val + get_abs_instr_count();
    }
  #endif // CONFIG_RV_CSR_MCOUNTINHIBIT_CNTR
}

static inline word_t csr_read(word_t *src) {
#ifdef CONFIG_RV_PMP_CSR
  if (is_pmpaddr(src)) {
    int idx = (src - &csr_array[CSR_PMPADDR_BASE]);
    if (idx >= CONFIG_RV_PMP_ACTIVE_NUM) {
      // CSRs of inactive pmp entries are read-only zero.
      return 0;
    }

    uint8_t cfg = pmpcfg_from_index(idx);
#ifdef CONFIG_SHARE
    if(dynamic_config.debug_difftest) {
      fprintf(stderr, "[NEMU] pmp addr read %d : 0x%016lx\n", idx,
        (cfg & PMP_A) >= PMP_NAPOT ? *src | (~pmp_tor_mask() >> 1) : *src & pmp_tor_mask());
    }
#endif // CONFIG_SHARE
    if ((cfg & PMP_A) >= PMP_NAPOT)
      return *src | (~pmp_tor_mask() >> 1);
    else
      return *src & pmp_tor_mask();
  }

  // No need to handle read pmpcfg specifically, because
  // - pmpcfg CSRs are all initialized to zero.
  // - writing to inactive pmpcfg CSRs is handled.
#endif // CONFIG_RV_PMP_CSR

#ifdef CONFIG_RVH
 if (cpu.v == 1) {

  if (is_read(sstatus))      { return gen_status_sd(vsstatus->val) | (vsstatus->val & SSTATUS_RMASK); }
  else if (is_read(sie))     { return get_v_sie(); }
  else if (is_read(stvec))   { return vstvec->val; }
  else if (is_read(sscratch)){ return vsscratch->val;}
  else if (is_read(sepc))    { return vsepc->val;}
  else if (is_read(scause))  { return vscause->val;}
  else if (is_read(stval))   { return vstval->val;}
  else if (is_read(sip))     { return (mip->val & VSI_MASK) >> 1;}
  else if (is_read(satp))    {
    if (cpu.mode == MODE_S && hstatus->vtvm == 1) {
      longjmp_exception(EX_VI);
    }else
      return vsatp->val;
  }
}
if (is_read(mideleg))        { return mideleg->val | MIDELEG_FORCED_MASK;}
if (is_read(hideleg))        { return hideleg->val & HIDELEG_MASK & (mideleg->val | MIDELEG_FORCED_MASK);}
if (is_read(hedeleg))        { return hedeleg->val & HEDELEG_MASK; }
if (is_read(hgeip))          { return hgeip->val & ~(0x1UL);}
if (is_read(hgeie))          { return hgeie->val & ~(0x1UL);}
if (is_read(hip))            { return mip->val & HIP_RMASK & (mideleg->val | MIDELEG_FORCED_MASK);}
if (is_read(hie))            { return mie->val & HIE_RMASK & (mideleg->val | MIDELEG_FORCED_MASK);}
if (is_read(hvip))           { return mip->val & HVIP_MASK;}
#ifdef CONFIG_RV_AIA
if (is_read(hvien))          { return hvien->val & HVIEN_MSAK; }
#endif
if (is_read(vsstatus))       { return gen_status_sd(vsstatus->val) | (vsstatus->val & SSTATUS_RMASK); }
if (is_read(vsip))           { return (mip->val & (hideleg->val & (mideleg->val | MIDELEG_FORCED_MASK)) & VSI_MASK) >> 1; }
if (is_read(vsie))           { return get_vsie(); }
#endif

  if (is_read(mstatus))     { return gen_status_sd(mstatus->val) | mstatus->val; }
  if (is_read(sstatus))     { return gen_status_sd(mstatus->val) | (mstatus->val & SSTATUS_RMASK); }
  else if (is_read(sie))    { return get_sie(); }
  else if (is_read(mtvec))  { return mtvec->val; }
  else if (is_read(stvec))  { return stvec->val; }
  else if (is_read(sip))    {
#ifndef CONFIG_RVH
    difftest_skip_ref();
#endif
    return mip->val & SIP_MASK;
  }
#ifdef CONFIG_RV_AIA
  else if (is_read(mvien))  { return mvien->val & MVIEN_MASK; }
#endif
#ifdef CONFIG_RVV
  else if (is_read(vcsr))   { return (vxrm->val & 0x3) << 1 | (vxsat->val & 0x1); }
  else if (is_read(vlenb))  { return VLEN >> 3; }
#endif
#ifndef CONFIG_FPU_NONE
  else if (is_read(fcsr))   {
    return fcsr->val & FCSR_MASK;
  }
  else if (is_read(fflags)) {
    return fcsr->fflags.val & FFLAGS_MASK;
  }
  else if (is_read(frm))    {
    return fcsr->frm & FRM_MASK;
  }
#endif // CONFIG_FPU_NONE
  else if (is_read(mcycle)) {
    // NEMU emulates a hart with CPI = 1.
    difftest_skip_ref();
    return get_mcycle();
  }
  else if (is_read(minstret)) {
    // The number of retired instruction should be the same between dut and ref.
    // But instruction counter of NEMU is not accurate when enabling Performance optimization.
    difftest_skip_ref();
    return get_minstret();
  }
#ifdef CONFIG_RV_ZICNTR
  else if (is_read(cycle)) {
    // NEMU emulates a hart with CPI = 1.
    difftest_skip_ref();
    return get_mcycle();
  }
  #ifdef CONFIG_RV_CSR_TIME
    else if (is_read(csr_time)) {
      difftest_skip_ref();
      return clint_uptime();
    }
  #endif // CONFIG_RV_CSR_TIME
  else if (is_read(instret)) {
    // The number of retired instruction should be the same between dut and ref.
    // But instruction counter of NEMU is not accurate when enabling Performance optimization.
    difftest_skip_ref();
    return get_minstret();
  }
#endif // CONFIG_RV_ZICNTR
#ifndef CONFIG_RVH
  if (is_read(mip)) { difftest_skip_ref(); }
#endif
  if (is_read(satp) && cpu.mode == MODE_S && mstatus->tvm == 1) { longjmp_exception(EX_II); }
#ifdef CONFIG_RV_SDTRIG
  if (is_read(tdata1)) { return cpu.TM->triggers[tselect->val].tdata1.val ^
    (cpu.TM->triggers[tselect->val].tdata1.mcontrol.hit << 20); }
  if (is_read(tdata2)) { return cpu.TM->triggers[tselect->val].tdata2.val; }
  if (is_read(tdata3)) { return cpu.TM->triggers[tselect->val].tdata3.val; }
#endif // CONFIG_RV_SDTRIG

#ifdef CONFIG_RV_SMSTATEEN
  if (is_read(mstateen0))   { return mstateen0->val; }
  if (is_read(sstateen0))   { return sstateen0->val & mstateen0->val; }
#ifdef CONFIG_RVH
  if (cpu.v == 1) {
  if (is_read(sstateen0)) { return sstateen0->val & hstateen0->val & mstateen0->val; }
}
  if (is_read(hstateen0))   { return hstateen0->val & mstateen0->val; }
#endif // CONFIG_RVH
#endif // CONFIG_RV_SMSTATEEN

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

#ifdef CONFIG_RVH
void update_vsatp(const vsatp_t new_val) {
  if (new_val.mode == SATP_MODE_BARE || new_val.mode == SATP_MODE_Sv39)
    vsatp->mode = new_val.mode;
  vsatp->asid = new_val.asid;
  switch (hgatp->mode) {
    case HGATP_MODE_BARE:
      vsatp->ppn = new_val.ppn & VSATP_PPN_HGATP_BARE_MASK;
      break;
    case HGATP_MODE_Sv39x4:
      vsatp->ppn = new_val.ppn & VSATP_PPN_HGATP_Sv39x4_MASK;
      break;
    default:
      panic("HGATP.mode is illegal value(%lx), when write vsatp\n", (uint64_t)hgatp->mode);
      break;
  }
}
#endif

static inline void csr_write(word_t *dest, word_t src) {
#ifdef CONFIG_RVH
  if(cpu.v == 1 && (is_write(sstatus) || is_write(sie) || is_write(stvec) || is_write(sscratch)
        || is_write(sepc) || is_write(scause) || is_write(stval) || is_write(sip)
        || is_write(satp) || is_write(stvec))){
    if (is_write(sstatus))      {
      vsstatus->val = mask_bitset(vsstatus->val, SSTATUS_WMASK, src);
    }
    else if (is_write(sie))     { set_v_sie(src); }
    else if (is_write(stvec))   { set_tvec((word_t*)vstvec, src); }
    else if (is_write(sscratch)){ vsscratch->val = src;}
    else if (is_write(sepc))    { vsepc->val = src;}
    else if (is_write(scause))  { vscause->val = src;}
    else if (is_write(stval))   { vstval->val = src;}
    else if (is_write(sip))     { mip->val = mask_bitset(mip->val, MIP_VSSIP, src << 1);}
    else if (is_write(satp))    {
      if (cpu.mode == MODE_S && hstatus->vtvm == 1) {
        longjmp_exception(EX_VI);
      }
      vsatp_t new_val = (vsatp_t)src;
      // legal mode
      if (new_val.mode == SATP_MODE_BARE || new_val.mode == SATP_MODE_Sv39) {
        update_vsatp(new_val);
      }
    }
  }
  else if (is_write(mideleg)){
    *dest = (src & MIDELEG_WMASK) | MIDELEG_FORCED_MASK;
  }
  else if (is_write(hideleg)) { hideleg->val = mask_bitset(hideleg->val, HIDELEG_MASK, src); }
  else if (is_write(hedeleg)) { hedeleg->val = mask_bitset(hedeleg->val, HEDELEG_MASK, src); }
  else if (is_write(hie)){
    mie->val = mask_bitset(mie->val, HIE_WMASK & (mideleg->val | MIDELEG_FORCED_MASK), src);
  }
  else if(is_write(hip)){
    mip->val = mask_bitset(mip->val, HIP_WMASK & (mideleg->val | MIDELEG_FORCED_MASK), src);
  }
  else if(is_write(hvip)){
    mip->val = mask_bitset(mip->val, HVIP_MASK, src);
  }
  #ifdef CONFIG_RV_AIA
  else if (is_write(hvien)) { hvien->val = mask_bitset(hvien->val, HVIEN_MSAK, src); }
  #endif
  else if(is_write(hstatus)){
    hstatus->val = mask_bitset(hstatus->val, HSTATUS_WMASK, src);
  }else if(is_write(vsstatus)){
    vsstatus->val = mask_bitset(vsstatus->val, SSTATUS_WMASK, src);
  }
  else if(is_write(vsie)){ set_vsie(src); }
  else if(is_write(vsip)){
    mip->val = mask_bitset(mip->val, MIP_VSSIP & (hideleg->val & (mideleg->val | MIDELEG_FORCED_MASK)), src << 1);
  }
  else if(is_write(vstvec)){
    set_tvec(dest, src);
  }
  else if(is_write(vsscratch)){
    vsscratch->val = src;
  }else if(is_write(vsepc)){
    vsepc->val = src;
  }else if(is_write(vscause)){
    vscause->val = src;
  }else if(is_write(vstval)){
    vstval->val = src;
  }else if(is_write(vsatp)){
    if (cpu.mode == MODE_S && hstatus->vtvm == 1) {
      longjmp_exception(EX_VI);
    }
    vsatp_t new_val = (vsatp_t)src;
    // Update vsatp without checking if vsatp.mode is legal, when hart is not in MODE_VS.
    update_vsatp(new_val);
  }else if (is_write(mstatus)) { mstatus->val = mask_bitset(mstatus->val, MSTATUS_WMASK, src); }
#ifdef CONFIG_RV_IMSIC
  else if (is_write(mtopi)) { return; }
  else if (is_write(stopi)) { return; }
  else if (is_write(vstopi)) { return; }
#endif // CONFIG_RV_IMSIC
#else
  if (is_write(mstatus)) {
#ifndef CONFIG_RVH
    unsigned prev_mpp = mstatus->mpp;
#endif // CONFIG_RVH
  if(mstatus->fs != 0){
    printf("write mstatus, fs valid, mstatus: %lx, write: %lx, mask: %lx\n", mstatus->val, src, MSTATUS_WMASK);
    mstatus->val = mask_bitset(mstatus->val, MSTATUS_WMASK, src);
    printf("after write, mstatus: %lx\n", mstatus->val);
  }else
    mstatus->val = mask_bitset(mstatus->val, MSTATUS_WMASK, src);
#ifndef CONFIG_RVH
    // Need to do an extra check for mstatus.MPP:
    // xPP fields are WARL fields that can hold only privilege mode x
    // and any implemented privilege mode lower than x.
    // M-mode software can determine whether a privilege mode is implemented
    // by writing that mode to MPP then reading it back. If the machine
    // provides only U and M modes, then only a single hardware storage bit
    // is required to represent either 00 or 11 in MPP.
    if (mstatus->mpp == MODE_HS) {
      // MODE_H is not implemented. The write will not take effect.
      mstatus->mpp = prev_mpp;
    }
#endif // CONFIG_RVH
  }
#endif // CONFIG_RVH
#ifdef CONFIG_RVH
  else if(is_write(hcounteren)){
    hcounteren->val = mask_bitset(hcounteren->val, COUNTEREN_MASK, src);
  }
#endif // CONFIG_RVH
  else if(is_write(scounteren)){
    scounteren->val = mask_bitset(scounteren->val, COUNTEREN_MASK, src);
  }
  else if(is_write(mcounteren)){
    mcounteren->val = mask_bitset(mcounteren->val, COUNTEREN_MASK, src);
  }
#ifdef CONFIG_RV_CSR_MCOUNTINHIBIT
  else if (is_write(mcountinhibit)) {
    update_counter_mcountinhibit(mcountinhibit->val, src & MCOUNTINHIBIT_MASK);
    mcountinhibit->val = mask_bitset(mcountinhibit->val, MCOUNTINHIBIT_MASK, src);
  }
#endif // CONFIG_RV_CSR_MCOUNTINHIBIT
  else if (is_write(mcycle)) {
    mcycle->val = set_mcycle(src);
  }
  else if (is_write(minstret)) {
    minstret->val = set_minstret(src);
  }
  else if (is_write(sstatus)) { if(mstatus->fs != 0){
    printf("write sstatus, fs valid, mstatus: %lx, write: %lx, mask: %lx\n", mstatus->val, src, SSTATUS_WMASK);
    mstatus->val = mask_bitset(mstatus->val, SSTATUS_WMASK, src);
    printf("after write, mstatus: %lx\n", mstatus->val);
  }else
    mstatus->val = mask_bitset(mstatus->val, SSTATUS_WMASK, src); }
  else if (is_write(sie)) { set_sie(src); }
  else if (is_write(mie)) { mie->val = mask_bitset(mie->val, MIE_MASK_BASE | MIE_MASK_H | LCOFI, src); }
  else if (is_write(mip)) {
    mip->val = mask_bitset(mip->val, MIP_MASK_BASE | MIP_MASK_H, src);
  }
  else if (is_write(sip)) { mip->val = mask_bitset(mip->val, ((cpu.mode == MODE_S) ? SIP_WMASK_S : SIP_MASK), src); }
#ifdef CONFIG_RV_AIA
  else if (is_write(mvien)) { mvien->val = mask_bitset(mvien->val, MVIEN_MASK, src); }
#endif
  else if (is_write(mtvec)) { set_tvec(dest, src); }
  else if (is_write(stvec)) { set_tvec(dest, src); }
  else if (is_write(medeleg)) { medeleg->val = mask_bitset(medeleg->val, MEDELEG_MASK, src); }
  else if (is_write(mideleg)) { *dest = src & 0x222; }
#ifdef CONFIG_RVV
  else if (is_write(vcsr)) { *dest = src & 0b111; vxrm->val = (src >> 1) & 0b11; vxsat->val = src & 0b1; }
  else if (is_write(vxrm)) { *dest = src & 0b11; vcsr->val = (vxrm->val) << 1 | vxsat->val; }
  else if (is_write(vxsat)) { *dest = src & 0b1; vcsr->val = (vxrm->val) << 1 | vxsat->val; }
  else if (is_write(vstart)) { *dest = src & (VLEN - 1); }
#endif
#ifdef CONFIG_MISA_UNCHANGEABLE
  else if (is_write(misa)) { /* do nothing */ }
#endif
  else if (is_write(mepc)) { *dest = src & (~0x1UL); }
  else if (is_write(sepc)) { *dest = src & (~0x1UL); }
#ifndef CONFIG_FPU_NONE
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
#endif // CONFIG_FPU_NONE
#ifdef CONFIG_RV_PMP_CSR
  else if (is_pmpaddr(dest)) {
    Logtr("Writing pmp addr");

    int idx = dest - &csr_array[CSR_PMPADDR_BASE];
    if (idx >= CONFIG_RV_PMP_ACTIVE_NUM) {
      // CSRs of inactive pmp entries are read-only zero.
      return;
    }

    word_t cfg = pmpcfg_from_index(idx);
    bool locked = cfg & PMP_L;
    // Note that the last pmp cfg do not have next_locked or next_tor
    bool next_locked = idx < (CONFIG_RV_PMP_ACTIVE_NUM - 1) && (pmpcfg_from_index(idx+1) & PMP_L);
    bool next_tor = idx < (CONFIG_RV_PMP_ACTIVE_NUM - 1) && (pmpcfg_from_index(idx+1) & PMP_A) == PMP_TOR;
    if (idx < CONFIG_RV_PMP_ACTIVE_NUM && !locked && !(next_locked && next_tor)) {
      *dest = src & (((word_t)1 << (CONFIG_PADDRBITS - PMP_SHIFT)) - 1);
    }
#ifdef CONFIG_SHARE
    if(dynamic_config.debug_difftest) {
      fprintf(stderr, "[NEMU] write pmp addr%d to %016lx\n",idx, *dest);
    }
#endif // CONFIG_SHARE

    mmu_tlb_flush(0);
  }
  else if (is_pmpcfg(dest)) {
    // Logtr("Writing pmp config");

    int idx_base = (dest - &csr_array[CSR_PMPCFG_BASE]) * 4;

    int xlen = 64;
    word_t cfg_data = 0;
    for (int i = 0; i < xlen / 8; i ++ ) {
      if (idx_base + i >= CONFIG_RV_PMP_ACTIVE_NUM) {
        // CSRs of inactive pmp entries are read-only zero.
        break;
      }
      word_t oldCfg = pmpcfg_from_index(idx_base + i);
#ifndef CONFIG_PMPTABLE_EXTENSION
      word_t cfg = ((src >> (i*8)) & 0xff) & (PMP_R | PMP_W | PMP_X | PMP_A | PMP_L);
#endif // CONFIG_PMPTABLE_EXTENSION
#ifdef CONFIG_PMPTABLE_EXTENSION
      /*
       * Consider the T-bit and C-bit of pmptable extension,
       * cancel original pmpcfg bit limit.
       */
      word_t cfg = ((src >> (i*8)) & 0xff);
#endif // CONFIG_PMPTABLE_EXTENSION
      if ((oldCfg & PMP_L) == 0) {
        cfg &= ~PMP_W | ((cfg & PMP_R) ? PMP_W : 0); // Disallow R=0 W=1
        if (CONFIG_PMP_GRANULARITY != PMP_SHIFT && (cfg & PMP_A) == PMP_NA4)
          cfg |= PMP_NAPOT; // Disallow A=NA4 when granularity > 4
        cfg_data |= (cfg << (i*8));
      } else {
        cfg_data |= (oldCfg << (i*8));
      }
    }
#ifdef CONFIG_SHARE
    if(dynamic_config.debug_difftest) {
      int idx = dest - &csr_array[CSR_PMPCFG_BASE];
      Logtr("[NEMU] write pmpcfg%d to %016lx\n", idx, cfg_data);
    }
#endif // CONFIG_SHARE

    *dest = cfg_data;

    mmu_tlb_flush(0);
  }
#endif // CONFIG_RV_PMP_CSR
  else if (is_write(satp)) {
    if (cpu.mode == MODE_S && mstatus->tvm == 1) {
      longjmp_exception(EX_II);
    }
    // Only support Sv39, ignore write that sets other mode
    if ((src & SATP_SV39_MASK) >> 60 == 8 || (src & SATP_SV39_MASK) >> 60 == 0)
      *dest = MASKED_SATP(src);
  }
#ifdef CONFIG_RV_SDTRIG
  else if (is_write(tselect)) {
    *dest = src < CONFIG_TRIGGER_NUM ? src : tselect->val;
    tdata1->val = cpu.TM->triggers[tselect->val].tdata1.val;
  } else if (is_write(tdata1)) {
    // not write to dest
    tdata1_t* tdata1_reg = &cpu.TM->triggers[tselect->val].tdata1.common;
    tdata1_t wdata = *(tdata1_t*)&src;
    switch (wdata.type)
    {
    case TRIG_TYPE_NONE: // write type 0 to disable this trigger
    case TRIG_TYPE_DISABLE:
      tdata1_reg->type = TRIG_TYPE_DISABLE;
      tdata1_reg->data = 0;
      break;
    case TRIG_TYPE_MCONTROL:
      mcontrol_checked_write(&cpu.TM->triggers[tselect->val].tdata1.mcontrol, &src, cpu.TM);
      tm_update_timings(cpu.TM);
      break;
    default:
      // do nothing for not supported trigger type
      break;
    }
    tdata1->val = cpu.TM->triggers[tselect->val].tdata1.val;
  } else if (is_write(tdata2)) {
    // not write to dest
    tdata2_t* tdata2_reg = &cpu.TM->triggers[tselect->val].tdata2;
    tdata2_t wdata = *(tdata2_t*)&src;
    tdata2_reg->val = wdata.val;
  }
#endif // CONFIG_RV_SDTRIG
#ifdef CONFIG_RV_SSCOFPMF
  else if (is_write(scountovf)) { *dest = src & SCOUNTOVF_WMASK; }
#endif // CONFIG_RV_SSCOFPMF

#ifdef CONFIG_RV_SMSTATEEN
  else if (is_write(mstateen0))   { *dest = ((src & MSTATEEN0_WMASK) | STATEEN0_CSRIND); }
  else if (is_write(sstateen0))   { *dest = (src & SSTATEEN0_WMASK); }
#ifdef CONFIG_RVH
    else if (is_write(hstateen0)) { *dest = ((src & HSTATEEN0_WMASK) | STATEEN0_CSRIND); }
#endif // CONFIG_RVH
#endif // CONFIG_RV_SMSTATEEN

#ifdef CONFIG_RVH
  else if (is_write(hgatp)) {
    hgatp_t new_val = (hgatp_t)src;
    // vmid and ppn WARL in the normal way, regardless of new_val.mode
    hgatp->vmid = new_val.vmid;
    // Make PPN[1:0] read only zero
    hgatp->ppn = new_val.ppn & ~(rtlreg_t)3 & BITMASK(CONFIG_PADDRBITS - PAGE_SHIFT);

    // Only support Sv39x4, ignore write that sets other mode
    if (new_val.mode == HGATP_MODE_Sv39x4 || new_val.mode == HGATP_MODE_BARE)
      hgatp->mode = new_val.mode;
    // When MODE=Bare, software should set the remaining fields in hgatp to zeros, not hardware.
  }
#endif// CONFIG_RVH
  else if (is_mhpmcounter(dest) || is_mhpmevent(dest)) {
    // read-only zero in NEMU
    return;
  }
  else { *dest = src; }

#ifndef CONFIG_FPU_NONE
  if (is_write(fflags) || is_write(frm) || is_write(fcsr)) {
    fp_set_dirty();
    fp_update_rm_cache(fcsr->frm);
  }
#endif

#ifdef CONFIG_RVV
  if (is_write(vcsr) || is_write(vstart) || is_write(vxsat) || is_write(vxrm)) {
    vp_set_dirty();
  }
#endif //CONFIG_RVV

#ifdef CONFIG_RVH
  if (is_write(mstatus) || is_write(satp) || is_write(vsatp) || is_write(hgatp)) { update_mmu_state(); }
  if (is_write(hstatus)) {
    set_sys_state_flag(SYS_STATE_FLUSH_TCACHE); // maybe change virtualization mode
  }
#else
  if (is_write(mstatus) || is_write(satp)) { update_mmu_state(); }
#endif
  if (is_write(satp)) { mmu_tlb_flush(0); } // when satp is changed(asid | ppn), flush tlb.
  if (is_write(mstatus) || is_write(sstatus) || is_write(satp) ||
      is_write(mie) || is_write(sie) || is_write(mip) || is_write(sip)) {
    set_sys_state_flag(SYS_STATE_UPDATE);
  }
}

word_t csrid_read(uint32_t csrid) {
  return csr_read(csr_decode(csrid));
}

// VS/VU access stateen should be EX_II when mstateen0->se0 is false.
// If smstateen check after csr_is_legal, the exception type will be wrong.
// todo: should finish all csr read/write exception checking before read/write.
#ifdef CONFIG_RV_SMSTATEEN
static inline void smstateen_extension_permit_check(word_t *dest_access) {
  if (is_access(sstateen0)) {
    if ((cpu.mode < MODE_M) && (!mstateen0->se0)) { longjmp_exception(EX_II); }
#ifdef CONFIG_RVH
    else if (cpu.v && mstateen0->se0 && !hstateen0->se0) { longjmp_exception(EX_VI); }
#endif // CONFIG_RVH
  }
#ifdef CONFIG_RVH
  else if (is_access(hstateen0)) {
    if ((cpu.mode < MODE_M) && (!mstateen0->se0)) { longjmp_exception(EX_II); }
    else if (cpu.v && mstateen0->se0) { longjmp_exception(EX_VI); }
  }
#endif // CONFIG_RVH
}
#endif // CONFIG_RV_SMSTATEEN

// AIA extension check
// !!! Only support in RVH
#ifdef CONFIG_RV_IMSIC
static void aia_extension_permit_check(word_t *dest_access) {
  if (is_access(stopei)) {
    if (!cpu.v && (cpu.mode == MODE_S) && mvien->seie) {
      longjmp_exception(EX_II);
    }
  }
  if (is_access(mireg)) {
    if (cpu.mode == MODE_M) {
      if ((miselect->val <= ISELECT_2F_MASK) ||
          ((miselect->val > ISELECT_3F_MASK) && (miselect->val <= ISELECT_6F_MASK)) ||
          (miselect->val >  ISELECT_MAX_MASK) ||
          (miselect->val & 0x1)) {
            longjmp_exception(EX_II);
      }
    }
  }
  if (is_access(sireg)) {
    if (!cpu.v && (cpu.mode == MODE_S) && mvien->seie) {
      if ((siselect->val > ISELECT_6F_MASK) && (siselect->val <= ISELECT_MAX_MASK)) {
        longjmp_exception(EX_II);
      }
    }
    if ((cpu.mode == MODE_M) || (!cpu.v && (cpu.mode == MODE_S))) {
      if ((siselect->val <= ISELECT_2F_MASK) ||
          ((siselect->val > ISELECT_3F_MASK) && (siselect->val <= ISELECT_6F_MASK)) ||
          (siselect->val >  ISELECT_MAX_MASK) ||
          (siselect->val & 0x1)) {
            longjmp_exception(EX_II);
      }
    }
    if (cpu.v && (cpu.mode == MODE_S)) {
      if (vsiselect->val > VSISELECT_MAX_MASK) {
        longjmp_exception(EX_II);
      }
      if (((vsiselect->val > ISELECT_2F_MASK) && (vsiselect->val <= ISELECT_3F_MASK)) ||
          ((vsiselect->val > 0x80) && (vsiselect->val <= ISELECT_MAX_MASK) && (vsiselect->val & 0x1))) {
            longjmp_exception(EX_VI);
      }
    }
    if (cpu.v && (cpu.mode == MODE_U)) {
      longjmp_exception(EX_VI);
    }
  }
  if (is_access(vsireg)) {
    if ((cpu.mode == MODE_M) || (!cpu.v && (cpu.mode == MODE_S))) {
      if ((vsiselect->val <= ISELECT_6F_MASK) ||
          (vsiselect->val >  ISELECT_MAX_MASK) ||
          (vsiselect->val & 0x1)) {
            longjmp_exception(EX_II);
      }
    }
    if (cpu.v) {
      longjmp_exception(EX_VI);
    }
  }
  if (is_access(sip) || is_access(sie)) {
    if (cpu.v && (cpu.mode == MODE_S)) {
      if (hvictl->vti) {
        longjmp_exception(EX_VI);
      }
    }
  }
}
#endif // CONFIG_RV_IMSIC

// Fp Vec CSR check
/**
 * Fp CSRs: fflags, frm, fcsr
 * Access fp CSRs raise EX_II
 *          1. when mstatus.FS is OFF in non Virt Mode
 *          2. when mstatus.FS or vsstatus.FS is OFF in Virt Mode
 *
 * Vec CSRs: vstart, vxsat, vxrm, vcsr, vl, vtype, vlenb
 * Access Vec CSRs raise EX_II
 *          1. when mstatus.VS is OFF in non Virt Mode
 *          2. when mstatus.VS or vsstatus.VS is OFF in Virt Mode
*/
#ifndef CONFIG_FPU_NONE
static inline void fp_permit_check(word_t *dest_access) {
  if (is_access(fcsr) || is_access(fflags) || is_access(frm)) {
    if (!require_fs()) { longjmp_exception(EX_II); }
  }
}
#endif // CONFIG_FPU_NONE

#ifdef CONFIG_RVV
static inline void vec_permit_check(word_t *dest_access) {
  if (is_access(vcsr) || is_access(vlenb) || is_access(vstart) || is_access(vxsat) || is_access(vxrm) || is_access(vl) || is_access(vtype)) {
    if (!require_vs()) { longjmp_exception(EX_II); }
  }
}
#endif // CONFIG_RVV

static void csrrw(rtlreg_t *dest, const rtlreg_t *src, uint32_t csrid) {
  word_t *csr = csr_decode(csrid);
#ifdef CONFIG_RV_SMSTATEEN
  smstateen_extension_permit_check(csr);
#endif // CONFIG_RV_SMSTATEEN
#ifdef CONFIG_RV_IMSIC
  aia_extension_permit_check(csr);
#endif // CONFIG_RV_IMSIC
#ifndef CONFIG_FPU_NONE
  fp_permit_check(csr);
#endif // CONFIG_FPU_NONE
#ifdef CONFIG_RVV
  vec_permit_check(csr);
#endif // CONFIG_RVV
  if (!csr_is_legal(csrid, src != NULL)) {
    Logti("Illegal csr id %u", csrid);
    longjmp_exception(EX_II);
    return;
  }
  // Log("Decoding csr id %u to %p", csrid, csr);
  word_t tmp = (src != NULL ? *src : 0);
  if (dest != NULL) { *dest = csr_read(csr); }
  if (src != NULL) { csr_write(csr, tmp); }
}

static word_t priv_instr(uint32_t op, const rtlreg_t *src) {
  switch (op) {
#ifndef CONFIG_MODE_USER
    case 0x102: // sret
#ifdef CONFIG_RVH
      if (cpu.v == 1){
        if((cpu.mode == MODE_S && hstatus->vtsr) || cpu.mode < MODE_S){
          longjmp_exception(EX_VI);
        }
        cpu.mode = vsstatus->spp;
        vsstatus->spp = MODE_U;
        vsstatus->sie = vsstatus->spie;
        vsstatus->spie = 1;
        return vsepc->val;
      }
#endif // CONFIG_RVH
      // cpu.v = 0
      if ((cpu.mode == MODE_S && mstatus->tsr) || cpu.mode < MODE_S) {
        longjmp_exception(EX_II);
      }
#ifdef CONFIG_RVH
      cpu.v = hstatus->spv;
      hstatus->spv = 0;
      set_sys_state_flag(SYS_STATE_FLUSH_TCACHE);
#endif //CONFIG_RVH
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
#ifdef CONFIG_RV_SDTRIG
      tcontrol->mte = tcontrol->mpte;
#endif
#ifdef CONFIG_RVH
      cpu.v = (mstatus->mpp == MODE_M ? 0 : mstatus->mpv);
      mstatus->mpv = 0;
      set_sys_state_flag(SYS_STATE_FLUSH_TCACHE);
#endif // CONFIG_RVH
      if (mstatus->mpp != MODE_M) { mstatus->mprv = 0; }
      mstatus->mpp = MODE_U;
      update_mmu_state();
      Loge("Executing mret to 0x%lx", mepc->val);
      return mepc->val;
      break;
#ifdef CONFIG_RV_SVINVAL
    case 0x180: // sfence.w.inval
    case 0x181: // sfence.inval.ir
      // in VU mode
      if (MUXDEF(CONFIG_RVH, cpu.v, false) && cpu.mode == MODE_U) {
        longjmp_exception(EX_VI);
      }
      // in U mode
      else if (cpu.mode == MODE_U) {
        longjmp_exception(EX_II);
      }
      break;
#endif // CONFIG_RV_SVINVAL
    case 0x105: // wfi
#ifdef CONFIG_RVH
      if((cpu.v && cpu.mode == MODE_S && hstatus->vtw == 1 && mstatus->tw == 0)
          ||(cpu.v && cpu.mode == MODE_U && mstatus->tw == 0)){
        longjmp_exception(EX_VI);
      }
#endif
      if ((cpu.mode < MODE_M && mstatus->tw == 1) || (cpu.mode == MODE_U)){
        longjmp_exception(EX_II);
      } // When S-mode is implemented, then executing WFI in U-mode causes an illegal instruction exception
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

#ifdef CONFIG_RVH
          if(cpu.v && (cpu.mode == MODE_U || (cpu.mode == MODE_S && hstatus->vtvm))) {
            longjmp_exception(EX_VI);
          }
          else if (!cpu.v && (cpu.mode == MODE_U || (cpu.mode == MODE_S && mstatus->tvm))) {
            longjmp_exception(EX_II);
          }
#else
          if ((cpu.mode == MODE_S && mstatus->tvm == 1) || cpu.mode == MODE_U)
            longjmp_exception(EX_II);
#endif // CONFIG_RVH
          mmu_tlb_flush(*src);
          break;
#ifdef CONFIG_RV_SVINVAL
        case 0x0b: // sinval.vma
#ifdef CONFIG_RVH
          if (!cpu.v && (cpu.mode == MODE_U || (cpu.mode == MODE_S && mstatus->tvm))) {
            longjmp_exception(EX_II);
          } else if (cpu.v && (cpu.mode == MODE_U || (cpu.mode == MODE_S && hstatus->vtvm))) {
            longjmp_exception(EX_VI);
          }
#else
          if ((cpu.mode == MODE_S && mstatus->tvm == 1) || cpu.mode == MODE_U)
            longjmp_exception(EX_II);
#endif // CONFIG_RVH
          mmu_tlb_flush(*src);
          break;
#endif // CONFIG_RV_SVINVAL
#ifdef CONFIG_RVH
        case 0x11: // hfence.vvma
          if(cpu.v) longjmp_exception(EX_VI);
          if(!cpu.v && cpu.mode == MODE_U) longjmp_exception(EX_II);
          mmu_tlb_flush(*src);
          break;
        case 0x31: // hfence.gvma
          if(cpu.v) longjmp_exception(EX_VI);
          if(!cpu.v && (cpu.mode == MODE_U || (cpu.mode == MODE_S && mstatus->tvm))) longjmp_exception(EX_II);
          mmu_tlb_flush(*src);
          break;
#ifdef CONFIG_RV_SVINVAL
        case 0x13: // hinval.vvma
          if(cpu.v) longjmp_exception(EX_VI);
          if(!cpu.v && cpu.mode == MODE_U) longjmp_exception(EX_II);
          mmu_tlb_flush(*src);
          break;
        case 0x33: // hinval.gvma
          if(cpu.v) longjmp_exception(EX_VI);
          if(!cpu.v && (cpu.mode == MODE_U || (cpu.mode == MODE_S && mstatus->tvm))) longjmp_exception(EX_II);
          mmu_tlb_flush(*src);
          break;
#endif // CONFIG_SVINVAL
#endif // CONFIG_RVH
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
    const rtlreg_t *src2, word_t imm) {
  word_t ret = 0;
  switch (id) {
    case HOSTCALL_CSR: csrrw(dest, src1, imm); return;
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
    case HOSTCALL_TRAP: ret = raise_intr(imm, *src1); break;
#endif
    case HOSTCALL_PRIV: ret = priv_instr(imm, src1); break;
    default: panic("Unsupported hostcall ID = %d", id);
  }
  if (dest) *dest = ret;
}

#ifdef CONFIG_RVH
int rvh_hlvx_check(struct Decode *s, int type){
  extern bool hlvx;
  hlvx = (s->isa.instr.i.opcode6_2 == 0x1c && s->isa.instr.i.funct3 == 0x4
                  && (s->isa.instr.i.simm11_0 == 0x643 || s->isa.instr.i.simm11_0 == 0x683));
  return hlvx;
}
extern bool hld_st;
int hload(Decode *s, rtlreg_t *dest, const rtlreg_t * src1, uint32_t id){
  if(cpu.v) longjmp_exception(EX_VI);
  if(!(cpu.mode == MODE_M || cpu.mode == MODE_S || (cpu.mode == MODE_U && hstatus->hu))){
    longjmp_exception(EX_II);
  }
  hld_st = true;
  int mmu_mode = get_h_mmu_state();
  switch (id) {
    case 0x600: // hlv.b
      rtl_lms(s, dest, src1, 0, 1, mmu_mode);
      break;
    case 0x601: // hlv.bu
      rtl_lm(s, dest, src1, 0, 1, mmu_mode);
      break;
    case 0x640: // hlv.h
      rtl_lms(s, dest, src1, 0, 2, mmu_mode);
      break;
    case 0x641: // hlv.hu
      rtl_lm(s, dest, src1, 0, 2, mmu_mode);
      break;
    case 0x643: // hlvx.hu
      rtl_lm(s, dest, src1, 0, 2, mmu_mode);
      break;
    case 0x680: // hlv.w
      rtl_lms(s, dest, src1, 0, 4, mmu_mode);
      break;
    case 0x681: // hlv.wu
      rtl_lm(s, dest, src1, 0, 4, mmu_mode);
      break;
    case 0x683: // hlvx.wu
      rtl_lm(s, dest, src1, 0, 4, mmu_mode);
      break;
    case 0x6c0: // hlv.d
      rtl_lms(s, dest, src1, 0, 8, mmu_mode);
      break;
    default:
#ifdef CONFIG_SHARE
      longjmp_exception(EX_II);
#else
      panic("Unsupported hypervisor vm load store operation = %d", id);
#endif // CONFIG_SHARE
  }
  hld_st = false;
  return 0;
}

int hstore(Decode *s, rtlreg_t *dest, const rtlreg_t * src1, const rtlreg_t * src2){
  if(cpu.v) longjmp_exception(EX_VI);
  if(!(cpu.mode == MODE_M || cpu.mode == MODE_S || (cpu.mode == MODE_U && hstatus->hu))){
    longjmp_exception(EX_II);
  }
  hld_st = true;
  uint32_t op = s->isa.instr.r.funct7;
  int mmu_mode = get_h_mmu_state();
  int len;
  switch (op) {
  case 0x31: // hsv.b
    len = 1;
    break;
  case 0x33: // hsv.h
    len = 2;
    break;
  case 0x35: // hsv.w
    len = 4;
    break;
  case 0x37: // hsv.d
    len = 8;
    break;
  default:
    #ifdef CONFIG_SHARE
      longjmp_exception(EX_II);
#else
      panic("Unsupported hypervisor vm load store operation = %d", op);
#endif // CONFIG_SHARE
  }
  rtl_sm(s, src2, src1, 0, len, mmu_mode);
  hld_st = false;
  return 0;
}
#endif
