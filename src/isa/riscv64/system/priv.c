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
#include "../local-include/aia.h"
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
inline word_t get_mip();

rtlreg_t csr_array[4096] = {};

#define T true
#define F false
#ifdef CONFIG_RVH
// [cpu.v][cpu.priv][csr priv]
bool access_table[2][4][4] = {
  {
    {T, F, F, F},
    {T, T, T, F},
    {F, F, F, F},
    {T, T, T, T}
  },
  {
    {T, F, F, F},
    {T, T, F, F},
    {F, F, F, F},
    {T, T, T, T},
  }
};
#else
// [cpu.priv][csr priv]
bool access_table[4][4] = {
  {T, F, F, F},
  {T, T, F, F},
  {F, F, F, F},
  {T, T, T, T}
};
#endif
#undef T
#undef F

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
  tinfo->val = 0
    IFDEF(CONFIG_TDATA1_ICOUNT, | (1 << TRIG_TYPE_ICOUNT))
    IFDEF(CONFIG_TDATA1_ITRIGGER, | (1 << TRIG_TYPE_ITRIG))
    IFDEF(CONFIG_TDATA1_ETRIGGER, | (1 << TRIG_TYPE_ETRIG))
    IFDEF(CONFIG_TDATA1_MCONTROL6, | (1 << TRIG_TYPE_MCONTROL6));
}
#endif // CONFIG_RV_SDTRIG

#ifdef CONFIG_RV_IMSIC
void init_iprio() {
  cpu.MIprios  = (IpriosModule*) malloc(sizeof (IpriosModule));
  cpu.SIprios  = (IpriosModule*) malloc(sizeof (IpriosModule));
  cpu.VSIprios = (IpriosModule*) malloc(sizeof (IpriosModule));
  cpu.MIpriosSort  = (IpriosSort*) malloc(sizeof (IpriosSort));
  cpu.SIpriosSort  = (IpriosSort*) malloc(sizeof (IpriosSort));
  cpu.VSIpriosSort = (IpriosSort*) malloc(sizeof (IpriosSort));
  cpu.HighestPrioIntr = (HighestPrioIntr*) malloc(sizeof (HighestPrioIntr));
  cpu.HighestPrioIntr->idx = 0;
  cpu.HighestPrioIntr->priority = 0;
  for (int i = 0; i < IPRIO_NUM; i++) {
    cpu.MIprios->iprios[i].val = 0;
    cpu.SIprios->iprios[i].val = 0;
    cpu.VSIprios->iprios[i].val = 0;
  }
  for (int i = 0; i < IPRIO_ENABLE_NUM; i++) {
    cpu.MIpriosSort->ipriosEnable[i].enable = false;
    cpu.SIpriosSort->ipriosEnable[i].enable = false;
    cpu.VSIpriosSort->ipriosEnable[i].enable = false;
    cpu.MIpriosSort->ipriosEnable[i].priority = 0;
    cpu.SIpriosSort->ipriosEnable[i].priority = 0;
    cpu.VSIpriosSort->ipriosEnable[i].priority = 0;
  }
}
#endif

// check s/h/mcounteren for counters, throw exception if counter is not enabled.
// also check h/mcounteren h/menvcfg for sstc
static inline bool csr_counter_enable_check(uint32_t addr) {
  bool has_vi = false;
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
      has_vi = true;
    }
  #endif // CONFIG_RVH

  if (cpu.mode < MODE_S && !(count_bit & scounteren->val) && !is_sstc_csr) {
    Logti("Illegal CSR accessing (0x%X): the bit in scounteren is not set", addr);
    if (MUXDEF(CONFIG_RVH, cpu.v, 0)) {
       has_vi = true;
    } else {
      longjmp_exception(EX_II);
    }
  }

  return has_vi;
}

static inline bool csr_normal_permit_check(uint32_t addr) {
  bool has_vi = false;
  assert(addr < 4096);
  // Attempts to access a non-existent CSR raise an illegal instruction exception.
  if(!csr_exist[addr]) {
    MUXDEF(CONFIG_PANIC_ON_UNIMP_CSR, panic("[NEMU] unimplemented CSR 0x%x", addr), longjmp_exception(EX_II));
  }

  // VS access Custom csr will cause EX_II
  #ifdef CONFIG_RVH
  bool is_custom_csr =  (addr >= 0x5c0 && addr <= 0x5ff) ||
                        (addr >= 0x9c0 && addr <= 0x9ff) ||
                        (addr >= 0xdc0 && addr <= 0xdff);
  if(cpu.v && cpu.mode == MODE_S && is_custom_csr){
    longjmp_exception(EX_II);
  }
  #endif // CONFIG_RVH

  // M/HS/VS/HU/VU access debug csr will cause EX_II
  bool isDebugReg = BITS(addr, 11, 4) == 0x7b; // addr(11,4)
  if(isDebugReg)
    longjmp_exception(EX_II);

  // Attempts to access a CSR without appropriate privilege level
  int csr_priv = BITS(addr, 9, 8); // get csr priv from csr addr
#ifdef CONFIG_RVH
  bool check_pass = access_table[cpu.v][cpu.mode][csr_priv];
#else
  bool check_pass = access_table[cpu.mode][csr_priv];
#endif  // CONFIG_RVH
  if (!check_pass) {
    if (MUXDEF(CONFIG_RVH, cpu.v && csr_priv != MODE_M, 0)) {
        has_vi = true;
    } else {
      longjmp_exception(EX_II);
    }
  }
  return has_vi;
}

static inline bool csr_readonly_permit_check(uint32_t addr, bool is_write) {
  // any mode write read-only csr will cause EX_II
  if (is_write && BITS(addr, 11, 10) == 0x3) {
    longjmp_exception(EX_II);
  }
  return false;
}

static inline word_t* csr_decode(uint32_t addr) {
  assert(addr < 4096);
  // Now we check if CSR is implemented / legal to access in csr_normal_permit_check()
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

#define MSTATUS_WMASK_MDT MUXDEF(CONFIG_RV_SMDBLTRP, (0X1UL << 42), 0)
#define MSTATUS_WMASK_SDT MUXDEF(CONFIG_RV_SSDBLTRP, (0x1UL << 24), 0)

// final mstatus wmask: dependent of the ISA extensions
#define MSTATUS_WMASK (    \
  MSTATUS_WMASK_BASE     | \
  MSTATUS_WMASK_FS       | \
  MSTATUS_WMASK_RVH      | \
  MSTATUS_WMASK_RVV      | \
  MSTATUS_WMASK_MDT      | \
  MSTATUS_WMASK_SDT        \
)

#define MSTATUS_RMASK_UBE 0X1UL << 6
// when harts without additional user extensions require new state XS read-only zero
#define MSTATUS_RMASK_XS  0X3UL << 15
#define MSTATUS_RMASK_UXL 0X3UL << 32
#define MSTATUS_RMASK_SXL 0X3UL << 34
#define MSTATUS_RMASK_SBE 0X1UL << 36
#define MSTATUS_RMASK_MBE 0X1UL << 37

#define MSTATUS_RMASK (   \
  MSTATUS_WMASK         | \
  MSTATUS_RMASK_UBE     | \
  MSTATUS_RMASK_XS      | \
  MSTATUS_RMASK_UXL     | \
  MSTATUS_RMASK_SXL     | \
  MSTATUS_RMASK_SBE     | \
  MSTATUS_RMASK_MBE       \
)

// wmask of sstatus is given by masking the valid fields in sstatus
#define SSTATUS_WMASK (MSTATUS_WMASK & SSTATUS_RMASK)

// hstatus wmask
#define HSTATUS_WMASK_GVA     (0x1UL << 6)
#define HSTATUS_WMASK_SPV     (0x1UL << 7)
#define HSTATUS_WMASK_SPVP    (0x1UL << 8)
#define HSTATUS_WMASK_HU      (0x1UL << 9)
#define HSTATUS_WMASK_VGEIN   (0x3fUL << 12)
#define HSTATUS_WMASK_VTVM    (0x1UL << 20)
#define HSTATUS_WMASK_VTM     (0x1UL << 21)
#define HSTATUS_WMASK_VTSR    (0x1UL << 22)

#define HSTATUS_WMASK (   \
  HSTATUS_WMASK_GVA     | \
  HSTATUS_WMASK_SPV     | \
  HSTATUS_WMASK_SPVP    | \
  HSTATUS_WMASK_HU      | \
  HSTATUS_WMASK_VGEIN   | \
  HSTATUS_WMASK_VTVM    | \
  HSTATUS_WMASK_VTM     | \
  HSTATUS_WMASK_VTSR      \
)

#define MENVCFG_RMASK_STCE    (0x1UL << 63)
#define MENVCFG_RMASK_PBMTE   (0x1UL << 62)
#define MENVCFG_RMASK_DTE     (0x1UL << 59)
#define MENVCFG_RMASK_CBZE    (0x1UL << 7)
#define MENVCFG_RMASK_CBCFE   (0x1UL << 6)
#define MENVCFG_RMASK_CBIE    (0x3UL << 4)
#define MENVCFG_RMASK (   \
  MENVCFG_RMASK_STCE    | \
  MENVCFG_RMASK_PBMTE   | \
  MENVCFG_RMASK_DTE     | \
  MENVCFG_RMASK_CBZE    | \
  MENVCFG_RMASK_CBCFE   | \
  MENVCFG_RMASK_CBIE      \
)

#define MENVCFG_WMASK_STCE    MUXDEF(CONFIG_RV_SSTC, MENVCFG_RMASK_STCE, 0)
#define MENVCFG_WMASK_PBMTE   MUXDEF(CONFIG_RV_SVPBMT, MENVCFG_RMASK_PBMTE, 0)
#define MENVCFG_WMASK_DTE     MUXDEF(CONFIG_RV_SSDBLTRP, MENVCFG_RMASK_DTE, 0)
#define MENVCFG_WMASK_CBZE    MUXDEF(CONFIG_RV_CBO, MENVCFG_RMASK_CBZE, 0)
#define MENVCFG_WMASK_CBCFE   MUXDEF(CONFIG_RV_CBO, MENVCFG_RMASK_CBCFE, 0)
#define MENVCFG_WMASK_CBIE    MUXDEF(CONFIG_RV_CBO, MENVCFG_RMASK_CBIE, 0)
#define MENVCFG_WMASK (    \
  MENVCFG_WMASK_STCE     | \
  MENVCFG_WMASK_PBMTE    | \
  MENVCFG_WMASK_DTE      | \
  MENVCFG_WMASK_CBZE     | \
  MENVCFG_WMASK_CBCFE    | \
  MENVCFG_WMASK_CBIE       \
)

#define SENVCFG_WMASK (    \
  MENVCFG_WMASK_CBZE     | \
  MENVCFG_WMASK_CBCFE    | \
  MENVCFG_WMASK_CBIE       \
)
#define HENVCFG_WMASK MENVCFG_WMASK

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

#define LCOFI MUXDEF(CONFIG_RV_SSCOFPMF, (1 << 13), 0)
#define LCI MUXDEF(CONFIG_RV_AIA, LCI_MASK, 0)

#ifdef CONFIG_RVH
#define HVIP_MASK     (VSI_MASK | MUXDEF(CONFIG_RV_SHLCOFIDELEG, MIP_LCOFIP, 0) | LCI)
#define HIP_RMASK     (MIP_VSTIP | MIP_VSEIP | MIP_SGEIP)
#define HIP_WMASK     MIP_VSSIP
#define HIE_RMASK     HSI_MASK
#define HIE_WMASK     HSI_MASK
#define HIDELEG_MASK  (VSI_MASK | MUXDEF(CONFIG_RV_SHLCOFIDELEG, MIP_LCOFIP, 0))
#define HEDELEG_MASK  ((1 << EX_IAM) | \
                       (1 << EX_IAF) | \
                       (1 << EX_II ) | \
                       (1 << EX_BP ) | \
                       (1 << EX_LAM) | \
                       (1 << EX_LAF) | \
                       (1 << EX_SAM) | \
                       (1 << EX_SAF) | \
                       (1 << EX_ECU) | \
                       (1 << EX_IPF) | \
                       (1 << EX_LPF) | \
                       (1 << EX_SPF) | \
                       (1 << EX_SWC) | \
                       (1 << EX_HWE))
#endif

#define MEDELEG_RVH ((1 << EX_IAM ) | \
                     (1 << EX_IAF ) | \
                     (1 << EX_II  ) | \
                     (1 << EX_BP  ) | \
                     (1 << EX_LAM ) | \
                     (1 << EX_LAF ) | \
                     (1 << EX_SAM ) | \
                     (1 << EX_SAF ) | \
                     (1 << EX_ECU ) | \
                     (1 << EX_ECS ) | \
                     (1 << EX_ECVS) | \
                     (1 << EX_IPF ) | \
                     (1 << EX_LPF ) | \
                     (1 << EX_SPF ) | \
                     (1 << EX_SWC ) | \
                     (1 << EX_HWE ) | \
                     (1 << EX_IGPF) | \
                     (1 << EX_LGPF) | \
                     (1 << EX_VI  ) | \
                     (1 << EX_SGPF))

#define MEDELEG_NONRVH ((1 << EX_IAM) | \
                        (1 << EX_IAF) | \
                        (1 << EX_II ) | \
                        (1 << EX_BP ) | \
                        (1 << EX_LAM) | \
                        (1 << EX_LAF) | \
                        (1 << EX_SAM) | \
                        (1 << EX_SAF) | \
                        (1 << EX_ECU) | \
                        (1 << EX_ECS) | \
                        (1 << EX_IPF) | \
                        (1 << EX_LPF) | \
                        (1 << EX_SPF) | \
                        (1 << EX_SWC) | \
                        (1 << EX_HWE))

#define MEDELEG_MASK MUXDEF(CONFIG_RVH,  MEDELEG_RVH, MEDELEG_NONRVH)


#define MIDELEG_WMASK_SSI (1 << 1)
#define MIDELEG_WMASK_STI (1 << 5)
#define MIDELEG_WMASK_SEI (1 << 9)
#define MIDELEG_WMASK_LCOFI MUXDEF(CONFIG_RV_SSCOFPMF, (1 << 13), 0)
#define MIDELEG_WMASK ( MIDELEG_WMASK_SSI | \
                        MIDELEG_WMASK_STI | \
                        MIDELEG_WMASK_SEI | \
                        MIDELEG_WMASK_LCOFI)

#define MIE_MASK_BASE 0xaaa
#define MIP_MASK_BASE (1 << 1)
#ifdef CONFIG_RVH
#define MIE_MASK_H ((1 << 2) | (1 << 6) | (1 << 10) | (1 << 12))
#define MIP_MASK_H MIP_VSSIP
#else
#define MIE_MASK_H 0
#define MIP_MASK_H 0
#endif // CONFIG_RVH

#define SIE_MASK_BASE (0x222 & mideleg->val)
#define SIP_MASK ((0x222 | LCOFI) & mideleg->val)
#define SIP_WMASK_S 0x2
#define MTIE_MASK (1 << 7)

// sie
#define SIE_LCOFI_MASK_MIE (mideleg->val & LCOFI)

// mvien
#define MVIEN_MASK (LCI | LCOFI | (1 << 9) | (1 << 1))
// hvien
#define HVIEN_MSAK (LCI | LCOFI)

// mvip
#define MVIP_MASK MUXDEF(CONFIG_RV_AIA, (LCOFI | LCI), 0)

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

#define MHPMEVENT_WMASK_OF      (0x1UL   << 63)
#define MHPMEVENT_WMASK_MINH    (0x1UL   << 62)
#define MHPMEVENT_WMASK_SINH    (0x1UL   << 61)
#define MHPMEVENT_WMASK_UINH    (0x1UL   << 60)
#define MHPMEVENT_WMASK_VSINH   (0x1UL   << 59)
#define MHPMEVENT_WMASK_VUINH   (0x1UL   << 58)
#define MHPMEVENT_WMASK_OPTYPE2 (0X1FUL  << 50)
#define MHPMEVENT_WMASK_OPTYPE1 (0X1FUL  << 45)
#define MHPMEVENT_WMASK_OPTYPE0 (0X1FUL  << 40)
#define MHPMEVENT_WMASK_EVENT3  (0X3FFUL << 30)
#define MHPMEVENT_WMASK_EVENT2  (0X3FFUL << 20)
#define MHPMEVENT_WMASK_EVENT1  (0X3FFUL << 10)
#define MHPMEVENT_WMASK_EVENT0  (0X3FFUL <<  0)

#define MHPMEVENT_WMASK (   \
  MHPMEVENT_WMASK_OF      | \
  MHPMEVENT_WMASK_MINH    | \
  MHPMEVENT_WMASK_SINH    | \
  MHPMEVENT_WMASK_UINH    | \
  MHPMEVENT_WMASK_VSINH   | \
  MHPMEVENT_WMASK_VUINH   | \
  MHPMEVENT_WMASK_OPTYPE2 | \
  MHPMEVENT_WMASK_OPTYPE1 | \
  MHPMEVENT_WMASK_OPTYPE0 | \
  MHPMEVENT_WMASK_EVENT3  | \
  MHPMEVENT_WMASK_EVENT2  | \
  MHPMEVENT_WMASK_EVENT1  | \
  MHPMEVENT_WMASK_EVENT0    \
)

#define is_read(csr) (src == (void *)(csr))
#define is_write(csr) (dest == (void *)(csr))
#define is_access(csr) (dest_access == (void *)(csr))
#define mask_bitset(old, mask, new) (((old) & ~(mask)) | ((new) & (mask)))

#define is_pmpcfg(p) (p >= &(csr_array[CSR_PMPCFG_BASE]) && p < &(csr_array[CSR_PMPCFG_BASE + CSR_PMPCFG_MAX_NUM]))
#define is_pmpaddr(p) (p >= &(csr_array[CSR_PMPADDR_BASE]) && p < &(csr_array[CSR_PMPADDR_BASE + CSR_PMPADDR_MAX_NUM]))
#define is_hpmcounter(p) (p >= &(csr_array[CSR_HPMCOUNTER_BASE]) && p < &(csr_array[CSR_HPMCOUNTER_BASE + CSR_HPMCOUNTER_NUM]))
#define is_mhpmcounter(p) (p >= &(csr_array[CSR_MHPMCOUNTER_BASE]) && p < &(csr_array[CSR_MHPMCOUNTER_BASE + CSR_MHPMCOUNTER_NUM]))
#define is_mhpmevent(p) (p >= &(csr_array[CSR_MHPMEVENT_BASE]) && p < &(csr_array[CSR_MHPMEVENT_BASE + CSR_MHPMEVENT_NUM]))

typedef enum {
  CPU_MODE_U = 0,
  CPU_MODE_VU,
  CPU_MODE_S,
  CPU_MODE_VS,
  CPU_MODE_M
} cpu_mode_t;

#ifdef CONFIG_RV_PMP_CSR
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
#endif // CONFIG_RV_PMP_CSR

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
  mstatus_t xstatus;
  xstatus.val = status;
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

static inline word_t gen_mask(word_t begin, word_t end) {
  word_t tmp_mask = 0;

  tmp_mask = ((1 << (end - begin + 1)) - 1) << begin;

  return tmp_mask;
}

static inline bool hpmevent_op_islegal(unsigned new_val) {
  switch (new_val) {
    case OP_OR: case OP_AND: case OP_XOR: case OP_ADD:
      return true;
    default:
      return false;
  }
}

#ifdef CONFIG_RV_AIA
static inline word_t vmode_get_ie(word_t old_value, word_t begin, word_t end) {
  word_t mask = gen_mask(begin, end);

  old_value |= mask & ((mie->val & mideleg->val & hideleg->val) |
                      (sie->val & (~mideleg->val & hideleg->val & mvien->val)) |
                      (vsie->val & (~hideleg->val & hvien->val)));

  return old_value;
}
#endif // CONFIG_RV_AIA

#ifdef CONFIG_RV_AIA
static inline void vmode_set_ie(word_t src, word_t begin, word_t end) {
  word_t mask = gen_mask(begin, end);
  sie->val = mask_bitset(sie->val, mask & (~mideleg->val & hideleg->val & mvien->val), src);
  vsie->val = mask_bitset(vsie->val, mask & (~hideleg->val & hvien->val), src);
}
#endif // CONFIG_RV_AIA

#ifdef CONFIG_RV_AIA
static inline word_t non_vmode_get_ie(word_t old_value, word_t begin, word_t end) {
  word_t mask = gen_mask(begin, end);

  old_value |= mask & ((mie->val & mideleg->val) |
                       (sie->val & (~mideleg->val & mvien->val)));

  return old_value;
}
#endif // CONFIG_RV_AIA

#ifdef CONFIG_RV_AIA
static inline void non_vmode_set_ie(word_t src, word_t begin, word_t end) {
  word_t mask = gen_mask(begin, end);
  sie->val = mask_bitset(sie->val, mask & (~mideleg->val & mvien->val), src);
}
#endif // CONFIG_RV_AIA

#ifdef CONFIG_RV_AIA
static inline word_t vmode_get_ip(word_t old_value, word_t begin, word_t end) {
  word_t mask = gen_mask(begin, end);

  old_value |= mask & ((get_mip() & (mideleg->val  & hideleg->val)) |
                      (mvip->val & (~mideleg->val & hideleg->val & mvien->val)) |
                      (hvip->val & (~hideleg->val & hvien->val)));
  
  return old_value;
}
#endif // CONFIG_RV_AIA

#ifdef CONFIG_RV_AIA
static inline void vmode_set_ip(word_t src, word_t begin, word_t end) {
  word_t mask = gen_mask(begin, end);
  mvip->val = mask_bitset(mvip->val, mask & (~mideleg->val & hideleg->val & mvien->val), src);
  hvip->val = mask_bitset(hvip->val, mask & (~hideleg->val & hvien->val), src);
}
#endif // CONFIG_RV_AIA

#ifdef CONFIG_RV_AIA
static inline word_t non_vmode_get_ip(word_t old_value, word_t begin, word_t end) {
  word_t mask = gen_mask(begin, end);

  old_value |= mask & ((get_mip() & mideleg->val) |
                      (mvip->val & (~mideleg->val & mvien->val)));

  return old_value;
}
#endif // CONFIG_RV_AIA

#ifdef CONFIG_RV_AIA
static inline void non_vmode_set_ip(word_t src, word_t begin, word_t end) {
  word_t mask = gen_mask(begin, end);
  mvip->val = mask_bitset(mvip->val, mask & (~mideleg->val & mvien->val), src);
}
#endif // CONFIG_RV_AIA

static inline word_t non_vmode_get_sie() {
  word_t tmp = 0;
#ifdef CONFIG_RV_AIA
  tmp |= mie->val & (MIP_SSIP | MIP_STIP | MIP_SEIP) & mideleg->val;
  tmp |= sie->val & (MIP_SSIP | MIP_SEIP) & (~mideleg->val & mvien->val);
  tmp |= non_vmode_get_ie(tmp, 13, 63);
#else
  tmp = mie->val & SIE_MASK_BASE;
  IFDEF(CONFIG_RV_SSCOFPMF, tmp |= mie->val & mideleg->val & MIP_LCOFIP);
#endif // CONFIG_RV_AIA

  return tmp;
}

static inline void non_vmode_set_sie(word_t src) {
#ifdef CONFIG_RV_AIA
  mie->val = mask_bitset(mie->val, (MIP_SSIP | MIP_STIP | MIP_SEIP | MIP_LCOFIP) & mideleg->val, src);
  sie->val = mask_bitset(sie->val, (MIP_SSIP | MIP_SEIP | MIP_LCOFIP) & (~mideleg->val & mvien->val), src);
  non_vmode_set_ie(src, 14, 63);
#else
#ifdef CONFIG_RV_SSCOFPMF
  mie->val = mask_bitset(mie->val, (SIE_MASK_BASE | SIE_LCOFI_MASK_MIE), src);
#else
  mie->val = mask_bitset(mie->val, SIE_MASK_BASE, src);
#endif // CONFIG_RV_SSCOFPMF
#endif // CONFIG_RV_AIA
}

static inline void set_tvec(word_t* dest, word_t src) {
  tvec_t newVal;
  newVal.val = src;
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
static inline word_t vmode_get_sie() {
  word_t tmp = 0;

  tmp = (mie->val & VSI_MASK) >> 1;

#ifdef CONFIG_RV_AIA
  tmp |= vmode_get_ie(tmp, 13, 63);
#else
  IFDEF(CONFIG_RV_SSCOFPMF, tmp |= mie->val & mideleg->val & hideleg->val & MIP_LCOFIP);
#endif // CONFIG_RV_AIA

  return tmp;
}
#endif // CONFIG_RVH

#ifdef CONFIG_RVH
static inline void vmode_set_sie(word_t src) {
  mie->val = mask_bitset(mie->val, VSI_MASK, src << 1);
#ifdef CONFIG_RV_AIA
  mie->val = mask_bitset(mie->val, MIP_LCOFIP & mideleg->val & hideleg->val, src);
  sie->val = mask_bitset(sie->val, MIP_LCOFIP & (~mideleg->val & hideleg->val & mvien->val), src);
  vsie->val = mask_bitset(vsie->val, MIP_LCOFIP & (~hideleg->val & hvien->val), src);
  vmode_set_ie(src, 14, 63);
#else
  IFDEF(CONFIG_RV_SSCOFPMF, mie->val = mask_bitset(mie->val, MIP_LCOFIP & mideleg->val & hideleg->val, src));
#endif // CONFIG_RV_AIA
}
#endif // CONFIG_RVH

#ifdef CONFIG_RVH
static inline word_t get_vsie() {
  word_t tmp;
  tmp = (mie->val & (hideleg->val & (mideleg->val | MIDELEG_FORCED_MASK)) & VSI_MASK) >> 1;

#ifdef CONFIG_RV_AIA
  tmp |= vmode_get_ie(tmp, 13, 63);
#else
  IFDEF(CONFIG_RV_SSCOFPMF, tmp |= mie->val & mideleg->val & hideleg->val & MIP_LCOFIP);
#endif // CONFIG_RV_AIA

  return tmp;
}
#endif // CONFIG_RVH

#ifdef CONFIG_RVH
static inline void set_vsie(word_t src) {
  mie->val = mask_bitset(mie->val, VSI_MASK & (hideleg->val & (mideleg->val | MIDELEG_FORCED_MASK)), src << 1);
#ifdef CONFIG_RV_AIA
  mie->val = mask_bitset(mie->val, MIP_LCOFIP & mideleg->val & hideleg->val, src);
  sie->val = mask_bitset(sie->val, MIP_LCOFIP & (~mideleg->val & hideleg->val & mvien->val), src);
  vsie->val = mask_bitset(vsie->val, MIP_LCOFIP & (~hideleg->val & hvien->val), src);
  vmode_set_ie(src, 14, 63);
#else
  IFDEF(CONFIG_RV_SSCOFPMF, mie->val = mask_bitset(mie->val, MIP_LCOFIP & mideleg->val & hideleg->val, src));
#endif // CONFIG_RV_AIA
}
#endif // CONFIG_RVH

#ifdef CONFIG_RVH
static inline word_t get_hie() {
  word_t tmp = 0;

  tmp = mie->val & HIE_RMASK & (mideleg->val | MIDELEG_FORCED_MASK);

  return tmp;
}
#endif

inline word_t get_mip() {
  word_t tmp = 0;

  tmp = mip->val & (MIP_MASK_BASE | MIP_LCOFIP);

  IFDEF(CONFIG_RVH, tmp |= hvip->val & (MIP_VSSIP));

  tmp |= cpu.non_reg_interrupt_pending.platform_irp_msip << 3;

#ifdef CONFIG_SHARE
#ifdef CONFIG_RV_SSTC
  if (menvcfg->stce) {
    tmp |= cpu.non_reg_interrupt_pending.platform_irp_stip << 5;
  } else {
    tmp |= mip->val & MIP_STIP;
  }
#else
  tmp |= mip->val & MIP_STIP;
#endif // CONFIG_RV_SSTC
#else
  tmp |= mip->val & (MIP_STIP | MIP_VSTIP | MIP_MTIP);
#endif

  IFDEF(CONFIG_RVH, tmp |= (hvip->vstip | cpu.non_reg_interrupt_pending.platform_irp_vstip) << 6);

  tmp |= cpu.non_reg_interrupt_pending.platform_irp_mtip << 7;

#ifdef CONFIG_RV_AIA
  if (mvien->seie) {
    tmp |= cpu.non_reg_interrupt_pending.platform_irp_seip << 9;
  } else {
    tmp |= (mvip->seip | cpu.non_reg_interrupt_pending.platform_irp_seip) << 9;
  }
#else
  tmp |= mip->val & MIP_SEIP;
#endif // CONFIG_RV_AIA

  IFDEF(CONFIG_RVH, tmp |= (hvip->vseip | cpu.non_reg_interrupt_pending.platform_irp_vseip) << 10);

  tmp |= cpu.non_reg_interrupt_pending.platform_irp_meip << 11;

  IFDEF(CONFIG_RVH, tmp |= ((hgeip->val & hgeie->val) != 0) << 12);

  return tmp;
}

static inline void set_mip(word_t src) {
  mip->val = mask_bitset(mip->val, MIP_MASK_BASE | LCOFI, src);

  IFDEF(CONFIG_RVH, hvip->val = mask_bitset(hvip->val, MIP_MASK_H, src));

#ifdef CONFIG_RV_SSTC
  if (!menvcfg->stce) {
    mip->val = mask_bitset(mip->val, MIP_STIP, src);
  }
#else
  mip->val = mask_bitset(mip->val, MIP_STIP, src);
#endif // CONFIG_RV_SSTC

#ifdef CONFIG_RV_AIA
  mvip->val = mask_bitset(mvip->val, MIP_SEIP & (~mvien->val), src);
#else
  mip->val = mask_bitset(mip->val, MIP_SEIP, src);
#endif // CONFIG_RV_AIA
}

static inline word_t non_vmode_get_sip() {
  word_t tmp = 0;
#ifdef CONFIG_RV_AIA
  tmp |= get_mip() & (MIP_SSIP | MIP_STIP | MIP_SEIP) & mideleg->val;
  tmp |= mvip->val & (MIP_SSIP | MIP_SEIP) & (~mideleg->val & mvien->val);
  tmp |= non_vmode_get_ip(tmp, 13, 63);
#else
  tmp = get_mip() & SIP_MASK;
#endif // CONFIG_RV_AIA
  return tmp;
}

static inline void non_vmode_set_sip(word_t src) {
#ifdef CONFIG_RV_AIA
  mip->val = mask_bitset(get_mip(), (MIP_SSIP | MIP_LCOFIP) & mideleg->val, src);
  mvip->val = mask_bitset(mvip->val, (MIP_SSIP | MIP_LCOFIP) & (~mideleg->val & mvien->val), src);
  non_vmode_set_ip(src, 14, 63);
#else
  mip->val = mask_bitset(get_mip(), ((cpu.mode == MODE_S) ? SIP_WMASK_S : SIP_MASK), src);
#endif // CONFIG_RV_AIA
}

#ifdef CONFIG_RV_AIA
static inline word_t get_mvip() {
  word_t tmp = 0;

  tmp = mvip->val & MVIP_MASK;

  tmp |= mvien->ssie ? mvip->val & MIP_SSIP : get_mip() & MIP_SSIP;

  if (!menvcfg->stce) {
    tmp |= get_mip() & MIP_STIP;
  }

  tmp |= mvip->val & MIP_SEIP;

  return tmp;
}
#endif // CONFIG_RV_AIA

#ifdef CONFIG_RV_AIA
static inline void set_mvip(word_t src) {
  mvip->val = mask_bitset(mvip->val, MVIP_MASK, src);

  mvip->val = mask_bitset(mvip->val, MIP_SSIP & mvien->val, src);

  mip->val = mask_bitset(get_mip(), MIP_SSIP & (~mvien->val), src);

  if (!menvcfg->stce) {
    mip->val = mask_bitset(get_mip(), MIP_STIP, src);
  }

  mvip->val = mask_bitset(mvip->val, MIP_SEIP, src);
}
#endif // CONFIG_RV_AIA

#ifdef CONFIG_RVH
static inline word_t vmode_get_sip() {
  word_t tmp = 0;

  tmp = (get_mip() & VSI_MASK) >> 1;

#ifdef CONFIG_RV_AIA
  tmp |= vmode_get_ip(tmp, 13, 63);
#else
  IFDEF(CONFIG_RV_SSCOFPMF, tmp |= get_mip() & mideleg->val & hideleg->val & MIP_LCOFIP);
#endif // CONFIG_RV_AIA

  return tmp;
}
#endif // CONFIG_RVH

#ifdef CONFIG_RVH
static inline void vmode_set_sip(word_t src) {
  hvip->val = mask_bitset(hvip->val, MIP_VSSIP, src << 1);

#ifdef CONFIG_RV_AIA
  mip->val = mask_bitset(get_mip(), MIP_LCOFIP & mideleg->val & hideleg->val, src);
  mvip->val = mask_bitset(mvip->val, MIP_LCOFIP & (~mideleg->val & hideleg->val & mvien->val), src);
  hvip->val = mask_bitset(hvip->val, MIP_LCOFIP & (~hideleg->val & hvien->val), src);
  vmode_set_ip(src, 14, 63);
#else
  IFDEF(CONFIG_RV_SSCOFPMF, mip->val = mask_bitset(get_mip(), MIP_LCOFIP & mideleg->val & hideleg->val, src));
#endif // CONFIG_RV_AIA
}
#endif // CONFIG_RVH

#ifdef CONFIG_RVH
static inline word_t get_vsip() {
  word_t tmp = 0;

  tmp = (hvip->val & (hideleg->val & (mideleg->val | MIDELEG_FORCED_MASK)) & VSI_MASK) >> 1;

#ifdef CONFIG_RV_AIA
  tmp |= vmode_get_ip(tmp, 13, 63);
#else
  IFDEF(CONFIG_RV_SSCOFPMF, tmp |= get_mip() & MIP_LCOFIP & mideleg->val & hideleg->val);
#endif // CONFIG_RV_AIA

  return tmp;
}
#endif // CONFIG_RVH

#ifdef CONFIG_RVH
static inline void set_vsip(word_t src) {
  hvip->val = mask_bitset(hvip->val, MIP_VSSIP & (hideleg->val & (mideleg->val | MIDELEG_FORCED_MASK)), src << 1);
#ifdef CONFIG_RV_AIA
  mip->val = mask_bitset(get_mip(), MIP_LCOFIP & mideleg->val & hideleg->val, src);
  mvip->val = mask_bitset(mvip->val, MIP_LCOFIP & (~mideleg->val & hideleg->val & mvien->val), src);
  hvip->val = mask_bitset(hvip->val, MIP_LCOFIP & (~hideleg->val & hvien->val), src);
  vmode_set_ip(src, 14, 63);
#else
  IFDEF(CONFIG_RV_SSCOFPMF, mip->val = mask_bitset(get_mip(), MIP_LCOFIP & mideleg->val & hideleg->val, src));
#endif // CONFIG_RV_AIA
}
#endif // CONFIG_RVH

#ifdef CONFIG_RVH
static inline word_t get_hip() {
  word_t tmp = 0;

  tmp = ((get_mip() & HIP_RMASK) | (hvip->val & MIP_VSSIP)) & (mideleg->val | MIDELEG_FORCED_MASK);

  return tmp;
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

#ifdef CONFIG_RV_IMSIC
inline void update_mtopi() {
  bool miprios_is_zero = iprio_is_zero(cpu.MIprios);
  uint64_t mtopi_gather = get_mip() & mie->val & (~(mideleg->val));
  bool mtopi_is_not_zero = mtopi_gather != 0;
  
  set_iprios_sort(mtopi_gather, cpu.MIpriosSort, cpu.MIprios);
  uint8_t m_iid_idx = high_iprio(cpu.MIpriosSort, IRQ_MEIP);
  uint8_t m_iid_num = interrupt_default_prio[m_iid_idx];
  uint8_t m_prio_num = cpu.MIpriosSort->ipriosEnable[m_iid_idx].priority;

  bool m_iid_default_prio_high_MEI = m_iid_idx < get_prio_idx_in_group(IRQ_MEIP);
  bool m_iid_default_prio_low_MEI = m_iid_idx > get_prio_idx_in_group(IRQ_MEIP);

  if (mtopi_is_not_zero) {
    mtopi->iid = m_iid_num;
    if (miprios_is_zero) {
      mtopi->iprio = 1;
    } else {
      if ((m_prio_num >= 1) && (m_prio_num <= 255)) {
        mtopi->iprio = m_prio_num;
      } else if ((m_prio_num > 255) || ((m_prio_num == 0) && (m_iid_default_prio_low_MEI))) {
        mtopi->iprio = 255;
      } else if ((m_prio_num == 0) && m_iid_default_prio_high_MEI) {
        mtopi->iprio = 0;
      }
    } 
  } else {
    mtopi->val = 0;
  }
}
#endif

#ifdef CONFIG_RV_IMSIC
inline void update_stopi() {
  bool siprios_is_zero = iprio_is_zero(cpu.SIprios);
  hip_t read_hip = (hip_t)get_hip();
  sip_t read_sip = (sip_t)non_vmode_get_sip();
  hie_t read_hie = (hie_t)get_hie();
  sie_t read_sie = (sie_t)non_vmode_get_sie();

  uint64_t stopi_gather = (read_hip.val | read_sip.val) & (read_hie.val | read_sie.val) & (~(hideleg->val));
  bool stopi_is_not_zero = stopi_gather != 0;
  set_iprios_sort(stopi_gather, cpu.SIpriosSort, cpu.SIprios);

  uint8_t s_iid_idx = high_iprio(cpu.SIpriosSort, IRQ_SEIP);
  uint8_t s_iid_num = interrupt_default_prio[s_iid_idx];
  uint8_t s_prio_num = cpu.SIpriosSort->ipriosEnable[s_iid_idx].priority;

  bool s_iid_default_prio_high_SEI = s_iid_idx < get_prio_idx_in_group(IRQ_SEIP);
  bool s_iid_default_prio_low_SEI = s_iid_idx > get_prio_idx_in_group(IRQ_SEIP);

  if (stopi_is_not_zero) {
    stopi->iid = s_iid_num;
    if (siprios_is_zero) {
      stopi->iprio = 1;
    } else {
      if ((s_prio_num >= 1) && (s_prio_num <= 255)) {
        stopi->iprio = s_prio_num;
      } else if ((s_prio_num > 255) || ((s_prio_num == 0) && (s_iid_default_prio_low_SEI))) {
        stopi->iprio = 255;
      } else if ((s_prio_num == 0) && s_iid_default_prio_high_SEI) {
        stopi->iprio = 0;
      }
    }
  } else {
    stopi->val = 0;
  }
}
#endif

#ifdef CONFIG_RV_IMSIC
inline void update_vstopi() {
  vsip_t read_vsip = (vsip_t)get_vsip();
  vsie_t read_vsie = (vsie_t)get_vsie();

  bool candidate1 = read_vsip.seip && read_vsie.seie && (hstatus->vgein != 0) && (cpu.xtopei.vstopei != 0);
  bool candidate2 = read_vsip.seip && read_vsie.seie && (hstatus->vgein == 0) && (hvictl->iid == 9) && (hvictl->iprio != 0);
  bool candidate3 = read_vsip.seip && read_vsie.seie && !candidate1 && !candidate2;
  bool candidate4 = !hvictl->vti && (read_vsie.val & read_vsip.val & 0xfffffffffffffdff);
  bool candidate5 = hvictl->vti && (hvictl->iid != 9);
  bool candidate_no_valid = !candidate1 && !candidate2 && !candidate3 && !candidate4 && !candidate5;

  uint64_t vstopi_gather = get_vsip() & get_vsie();
  set_iprios_sort(vstopi_gather, cpu.VSIpriosSort, cpu.VSIprios);
  set_viprios_sort(vstopi_gather);

  uint8_t vs_iid_idx = high_iprio(cpu.VSIpriosSort, IRQ_VSEIP);
  uint8_t vs_iid_num = interrupt_default_prio[vs_iid_idx];
  uint8_t vs_prio_num = cpu.VSIpriosSort->ipriosEnable[vs_iid_idx].priority;

  uint8_t iid_candidate123 = IRQ_SEIP;
  uint8_t iid_candidate45 = 0;
  uint16_t iprio_candidate123 = 0;
  uint16_t iprio_candidate45 = 0;

  if (candidate1) {
    vstopei_t* vstopei_tmp = (vstopei_t*)cpu.xtopei.vstopei;
    iprio_candidate123 = vstopei_tmp->iprio;
  } else if (candidate2) {
    iprio_candidate123 = hvictl->iprio;
  } else if (candidate3) {
    iprio_candidate123 = 256;
  }

  if (candidate4) {
    iid_candidate45 = vs_iid_num;
    iprio_candidate45 = vs_prio_num;
  } else if (candidate5) {
    iid_candidate45 = hvictl->iid;
    iprio_candidate45 = hvictl->iprio;
  }

  bool candidate123 = candidate1 || candidate2 || candidate3;
  bool candidate45 = candidate4 || candidate5;
  bool candidate123_high_candidate45 = false;
  bool candidate123_low_candidate45 = false;

  if (candidate123 && candidate4) {
    candidate123_high_candidate45 = (iprio_candidate123 < iprio_candidate45) || ((iprio_candidate123 == iprio_candidate45) && (get_prio_idx_in_group(iid_candidate123) <= get_prio_idx_in_group(iid_candidate45)));
    candidate123_low_candidate45  = (iprio_candidate123 > iprio_candidate45) || ((iprio_candidate123 == iprio_candidate45) && (get_prio_idx_in_group(iid_candidate123) > get_prio_idx_in_group(iid_candidate45)));
  } else if (candidate123 && candidate5) {
    candidate123_high_candidate45 = (iprio_candidate123 < iprio_candidate45) || ((iprio_candidate123 == iprio_candidate45) && hvictl->dpr);
    candidate123_low_candidate45  = (iprio_candidate123 > iprio_candidate45) || ((iprio_candidate123 == iprio_candidate45) && !hvictl->dpr);
  } else if (candidate123 && !candidate45) {
    candidate123_high_candidate45 = true;
  } else if (!candidate123 && candidate45) {
    candidate123_low_candidate45 = true;
  }

  uint8_t iid_candidate = 0;
  uint16_t iprio_candidate = 0;

  if (candidate123_high_candidate45) {
    iid_candidate = iid_candidate123;
    iprio_candidate = iprio_candidate123;
  } else if (candidate123_low_candidate45) {
    iid_candidate = iid_candidate45;
    iprio_candidate = iprio_candidate45;
  }

  if (candidate_no_valid) {
    vstopi->val = 0;
  } else {
    vstopi->iid = iid_candidate;
    if (iprio_candidate > 255) {
      vstopi->iprio = 255;
    } else if (candidate123_low_candidate45 && candidate5 && !hvictl->ipriom) {
      vstopi->iprio = 1;
    } else if ((candidate123_high_candidate45 && (iprio_candidate <= 255)) || (candidate123_low_candidate45 && candidate4) || (candidate123_low_candidate45 && candidate5 && hvictl->ipriom)) {
      vstopi->iprio = iprio_candidate & 0xff;
    }
  }
}
#endif

#ifdef CONFIG_RV_IMSIC
bool iselect_is_major_ip(uint64_t iselect) {
  return (iselect > ISELECT_2F_MASK) && (iselect <= ISELECT_3F_MASK) && !(iselect & 0x1);
}
#endif

static word_t csr_read(uint32_t csrid) {
  word_t *src = csr_decode(csrid);
  switch (csrid) {
    /************************* Unprivileged and User-Level CSRs *************************/
#ifndef CONFIG_FPU_NONE
    case CSR_FFLAGS: return fcsr->fflags.val & FFLAGS_MASK;
    case CSR_FRM: return fcsr->frm & FRM_MASK;
    case CSR_FCSR: return fcsr->val & FCSR_MASK;
#endif // CONFIG_FPU_NONE

#ifdef CONFIG_RVV
    case CSR_VCSR: return (vxrm->val & 0x3) << 1 | (vxsat->val & 0x1);
#endif // CONFIG_RVV

#ifdef CONFIG_RV_ZICNTR
    case CSR_CYCLE:
      // NEMU emulates a hart with CPI = 1.
      difftest_skip_ref();
      return get_mcycle();
#ifdef CONFIG_RV_CSR_TIME
    case CSR_TIME:
      difftest_skip_ref();
      return clint_uptime();
#endif // CONFIG_RV_CSR_TIME
    case CSR_INSTRET:
      // The number of retired instruction should be the same between dut and ref.
      // But instruction counter of NEMU is not accurate when enabling Performance optimization.
      difftest_skip_ref();
      return get_minstret();
#endif // CONFIG_RV_ZICNTR

#ifdef CONFIG_RVV
    case CSR_VLENB: return VLEN >> 3;
#endif // CONFIG_RVV

    /************************* Supervisor-Level CSRs *************************/
    case CSR_SSTATUS:
#ifdef CONFIG_RVH
      if (cpu.v) {
        uint64_t vsstatus_rmask = SSTATUS_RMASK;
#ifdef CONFIG_RV_SSDBLTRP
        vsstatus_rmask &= ((menvcfg->dte & henvcfg->dte) ? vsstatus_rmask : ~MSTATUS_WMASK_SDT);
#endif // CONFIG_RV_SSDBLTRP
        return gen_status_sd(vsstatus->val) | (vsstatus->val & vsstatus_rmask);
      }
#endif // CONFIG_RVH

      uint64_t sstatus_rmask = SSTATUS_RMASK;
#ifdef CONFIG_RV_SSDBLTRP
      sstatus_rmask &= (menvcfg->dte ? sstatus_rmask : ~MSTATUS_WMASK_SDT);
#endif //CONFIG_RV_SSDBLTRP
      return gen_status_sd(mstatus->val) | (mstatus->val & sstatus_rmask);

#ifdef CONFIG_RV_SMSTATEEN
    case CSR_SSTATEEN0:
      IFDEF(CONFIG_RVH, if (cpu.v) return sstateen0->val & hstateen0->val & mstateen0->val);
      return sstateen0->val & mstateen0->val;
#endif // CONFIG_RV_SMSTATEEN

    case CSR_SIE:
      IFDEF(CONFIG_RVH, if (cpu.v) return vmode_get_sie());
      return non_vmode_get_sie();
    case CSR_STVEC: 
      IFDEF(CONFIG_RVH, if (cpu.v) return vstvec->val);
      return stvec->val;
    case CSR_SSCRATCH:
      IFDEF(CONFIG_RVH, if (cpu.v) return vsscratch->val);
      return sscratch->val;
    case CSR_SEPC:
      IFDEF(CONFIG_RVH, if (cpu.v) return vsepc->val);
      return sepc->val;
    case CSR_SCAUSE:
      IFDEF(CONFIG_RVH, if (cpu.v) return vscause->val);
      return scause->val;
    case CSR_STVAL:
      IFDEF(CONFIG_RVH, if (cpu.v) return vstval->val);
      return stval->val;
    case CSR_SIP:
      IFDEF(CONFIG_RVH, if (cpu.v) return vmode_get_sip());
      IFNDEF(CONFIG_RVH, difftest_skip_ref());
      return non_vmode_get_sip();
#ifdef CONFIG_RV_SSTC
    case CSR_STIMECMP:
      IFDEF(CONFIG_RVH, if (cpu.v) return vstimecmp->val);
      return stimecmp->val;
#endif // CONFIG_RV_SSTC
#ifdef CONFIG_RV_IMSIC
    case CSR_SISELECT:
      IFDEF(CONFIG_RVH, if (cpu.v) return vsiselect->val);
      return siselect->val;
    case CSR_STOPI:
      if (cpu.v) return vstopi->val;
      return stopi->val;
    case CSR_STOPEI:
      if (cpu.v) return cpu.xtopei.vstopei;
      return cpu.xtopei.stopei;
    case CSR_SIREG:
    {
      bool siselect_is_major_ip = iselect_is_major_ip(siselect->val);
      if (siselect_is_major_ip) {
        return cpu.SIprios->iprios[(siselect->val - ISELECT_2F_MASK - 1) >> 1].val;
      }
      return 0;
    }
#endif // CONFIG_RV_IMSIC
    case CSR_SATP:
      IFDEF(CONFIG_RVH, if (cpu.v) return vsatp->val);
      return satp->val;

    /************************* Hypervisor and VS CSRs *************************/
#ifdef CONFIG_RVH
    case CSR_VSSTATUS:
    {
      uint64_t vsstatus_rmask = SSTATUS_RMASK;
#ifdef CONFIG_RV_SSDBLTRP
      vsstatus_rmask &= ((menvcfg->dte & henvcfg->dte) ? vsstatus_rmask : ~MSTATUS_WMASK_SDT);
#endif //CONFIG_RV_SSDBLTRP
      return gen_status_sd(vsstatus->val) | (vsstatus->val & vsstatus_rmask);
    }

    case CSR_VSIE: return get_vsie();
    case CSR_VSIP: return get_vsip();
    case CSR_HEDELEG: return hedeleg->val & HEDELEG_MASK;
    case CSR_HIDELEG: return hideleg->val & HIDELEG_MASK;
    case CSR_HIE: return get_hie();
    case CSR_HGEIE: return hgeie->val & ~(0x1UL);
#ifdef CONFIG_RV_AIA
    case CSR_HVIEN: return hvien->val & HVIEN_MSAK;
#endif
    case CSR_HENVCFG:
    {
      uint64_t henvcfg_out = henvcfg->val;
      henvcfg_out &= menvcfg->val & MENVCFG_WMASK;
      /* henvcfg.stce/dte/pbmte is read_only 0 when menvcfg.stce/dte/pbmte = 0 */
      henvcfg_out &= menvcfg->val | ~(MENVCFG_RMASK_STCE | MENVCFG_RMASK_DTE | MENVCFG_RMASK_PBMTE);
      return henvcfg_out & HENVCFG_WMASK;
    }

#ifdef CONFIG_RV_SMSTATEEN
    case CSR_HSTATEEN0: return hstateen0->val & mstateen0->val;
#endif // CONFIG_RV_SMSTATEEN

    case CSR_HIP: return get_hip();
    case CSR_HVIP: return hvip->val & HVIP_MASK;
    case CSR_HGEIP: return hgeip->val & ~(0x1UL);
#ifdef CONFIG_RV_IMSIC
    case CSR_VSTOPEI: return cpu.xtopei.vstopei;
    case CSR_VSIREG:
    {
      bool vsiselect_is_major_ip = iselect_is_major_ip(siselect->val);
      if (vsiselect_is_major_ip) {
        return cpu.SIprios->iprios[(siselect->val - ISELECT_2F_MASK - 1) >> 1].val;
      }
      return 0;
    }
#endif
#endif // CONFIG_RVH

    /************************* Machine-Level CSRs *************************/
    case CSR_MSTATUS:
    {
      uint64_t mstatus_rmask = MSTATUS_RMASK;
#ifdef CONFIG_RV_SSDBLTRP
      mstatus_rmask &= (menvcfg->dte ? mstatus_rmask : ~MSTATUS_WMASK_SDT);
#endif //CONFIG_RV_SSDBLTRP
      return gen_status_sd(mstatus->val) | (mstatus->val & mstatus_rmask);
    }

#ifdef CONFIG_RV_AIA
    case CSR_MVIEN: return mvien->val & MVIEN_MASK;
    case CSR_MVIP: return get_mvip();
#ifdef CONFIG_RV_IMSIC
    case CSR_MTOPEI: return cpu.xtopei.mtopei;
    case CSR_MIREG:
    {
      bool miselect_is_major_ip = iselect_is_major_ip(miselect->val);
      if (miselect_is_major_ip) {
        return cpu.MIprios->iprios[(miselect->val - ISELECT_2F_MASK - 1) >> 1].val;
      }
      return 0;
    }
#endif
#endif // CONFIG_RV_AIA

    case CSR_MIP:
#ifndef CONFIG_RVH
        difftest_skip_ref();
        return mip->val;
#else 
        return get_mip();
#endif

#ifdef CONFIG_RV_PMP_CSR
    case CSR_PMPADDR_BASE ... CSR_PMPADDR_BASE+CSR_PMPADDR_MAX_NUM-1:
    {
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


    // No need to handle read pmpcfg specifically, because
    // - pmpcfg CSRs are all initialized to zero.
    // - writing to inactive pmpcfg CSRs is handled.
    }
#endif // CONFIG_RV_PMP_CSR

#ifdef CONFIG_RV_SMRNMI
    case CSR_MNEPC: return mnepc->val & (~0x1UL) ;
    case CSR_MNSTATUS: return mnstatus->val & MNSTATUS_MASK;
#endif // CONFIG_RV_SMRNMI

#ifdef CONFIG_RV_SDTRIG
    case CSR_TDATA1: return get_tdata1(cpu.TM);
    case CSR_TDATA2: return get_tdata2(cpu.TM);
#ifdef CONFIG_SDTRIG_EXTRA
    case CSR_TDATA3: return get_tdata3(cpu.TM);
#endif // CONFIG_SDTRIG_EXTRA
#endif // CONFIG_RV_SDTRIG

    case CSR_MCYCLE: 
      // NEMU emulates a hart with CPI = 1.
      difftest_skip_ref();
      return get_mcycle();
    case CSR_MINSTRET:
      // The number of retired instruction should be the same between dut and ref.
      // But instruction counter of NEMU is not accurate when enabling Performance optimization.
      difftest_skip_ref();
      return get_minstret();
    /************************* All Others Normal CSRs *************************/
    default: return *src;
  }
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
    Log("Disabled machine/supervisor/virtualized-supervisor time interruption\n");
    mie->mtie = 0;
    mie->stie = 0;
    mie->vstie = 0;
}

#ifdef CONFIG_RVH
void update_vsatp(const vsatp_t new_val) {
#ifdef CONFIG_RV_SV48
  if (new_val.mode == SATP_MODE_BARE || new_val.mode == SATP_MODE_Sv39 || new_val.mode == SATP_MODE_Sv48)
#else
  if (new_val.mode == SATP_MODE_BARE || new_val.mode == SATP_MODE_Sv39)
#endif // CONFIG_RV_SV48
    vsatp->mode = new_val.mode;
  vsatp->asid = new_val.asid;
  switch (hgatp->mode) {
    case HGATP_MODE_BARE:
      vsatp->ppn = new_val.ppn & VSATP_PPN_HGATP_BARE_MASK;
      break;
    case HGATP_MODE_Sv39x4:
      vsatp->ppn = new_val.ppn & VSATP_PPN_HGATP_Sv39x4_MASK;
      break;
#ifdef CONFIG_RV_SV48
    case HGATP_MODE_Sv48x4:
      vsatp->ppn = new_val.ppn & VSATP_PPN_HGATP_Sv48x4_MASK;
      break;
#endif // CONFIG_RV_SV48
    default:
      panic("HGATP.mode is illegal value(%lx), when write vsatp\n", (uint64_t)hgatp->mode);
      break;
  }
}
#endif

static void csr_write(uint32_t csrid, word_t src) {
  word_t *dest = csr_decode(csrid);
  switch (csrid) {
    /************************* Unprivileged and User-Level CSRs *************************/
#ifndef CONFIG_FPU_NONE
    case CSR_FFLAGS:
      *dest = src & FFLAGS_MASK;
      fcsr->val = (frm->val)<<5 | fflags->val;
      break;
    case CSR_FRM:
      *dest = src & FRM_MASK;
      fcsr->val = (frm->val)<<5 | fflags->val;
      break;
    case CSR_FCSR:
      *dest = src & FCSR_MASK;
      fflags->val = src & FFLAGS_MASK;
      frm->val = ((src)>>5) & FRM_MASK;
      break;
#endif // CONFIG_FPU_NONE

#ifdef CONFIG_RVV
    case CSR_VSTART: *dest = src & (VLEN - 1); break;
    case CSR_VXSAT: *dest = src & 0b1; vcsr->val = (vxrm->val) << 1 | vxsat->val; break;
    case CSR_VXRM: *dest = src & 0b11; vcsr->val = (vxrm->val) << 1 | vxsat->val; break;
    case CSR_VCSR: *dest = src & 0b111; vxrm->val = (src >> 1) & 0b11; vxsat->val = src & 0b1; break;
#endif // CONFIG_RVV

    /************************* Supervisor-Level CSRs *************************/
    case CSR_SSTATUS:
#ifdef CONFIG_RVH
      if (cpu.v) {
        uint64_t sstatus_wmask = SSTATUS_WMASK;
#ifdef CONFIG_RV_SSDBLTRP
        // when menvcfg or henvcfg.DTE close,  vsstatus.SDT is read-only
        if (menvcfg->dte == 0 || henvcfg->dte == 0) {
          src &= sstatus_wmask & (~MSTATUS_WMASK_SDT);
        }
        // the same as mstatus SIE
        if (src & MSTATUS_SIE) {
          sstatus_wmask &= ~MSTATUS_SIE;
          if (((src & MSTATUS_WMASK_SDT) == 0) || ( vsstatus->sdt == 0)) {
            sstatus_wmask |= MSTATUS_SIE;
          }
        }
#endif //CONFIG_RV_SSDBLTRP
        vsstatus->val = mask_bitset(vsstatus->val, sstatus_wmask, src);
#ifdef CONFIG_RV_SSDBLTRP
        if (src & MSTATUS_WMASK_SDT) { vsstatus->sie = 0; }
#endif //CONFIG_RV_SSDBLTRP
        break;
      }
#endif // CONFIG_RVH
      uint64_t sstatus_wmask = SSTATUS_WMASK;
#ifdef CONFIG_RV_SSDBLTRP
      // when menvcfg or henvcfg.DTE close,  vsstatus.SDT is read-only
      if (menvcfg->dte == 0 ) {
        src &= sstatus_wmask & (~MSTATUS_WMASK_SDT);
      }
      // the same as mstatus SIE
      if (src & MSTATUS_SIE) {
        sstatus_wmask &= ~MSTATUS_SIE;
        if (((src & MSTATUS_WMASK_SDT) == 0) || ( mstatus->sdt == 0)) {
          sstatus_wmask |= MSTATUS_SIE;
        }
      }
#endif //CONFIG_RV_SSDBLTRP
      mstatus->val = mask_bitset(mstatus->val, sstatus_wmask, src); // xiangshan pass mstatus.rdata ,so clear mstatus->sdt
#ifdef CONFIG_RV_SSDBLTRP
      if (src & MSTATUS_WMASK_SDT) { mstatus->sie = 0; }
#endif //CONFIG_RV_SSDBLTRP
      break;

    case CSR_SCOUNTEREN: scounteren->val = mask_bitset(scounteren->val, COUNTEREN_MASK, src); break;

    case CSR_SENVCFG:
      senvcfg->val = mask_bitset(senvcfg->val, SENVCFG_WMASK & (~MENVCFG_WMASK_CBIE), src);
      if (((senvcfg_t*)&src)->cbie != 0b10) { // 0b10 is reserved
        senvcfg->val = mask_bitset(senvcfg->val, MENVCFG_WMASK_CBIE, src);
      }
      break;

#ifdef CONFIG_RV_SMSTATEEN
    case CSR_SSTATEEN0: *dest = (src & SSTATEEN0_WMASK); break;
#endif // CONFIG_RV_SMSTATEEN

    case CSR_SIE:
      IFDEF(CONFIG_RVH, if (cpu.v) {vmode_set_sie(src); break;});
      non_vmode_set_sie(src);
      break;

    case CSR_STVEC:
      IFDEF(CONFIG_RVH, if (cpu.v) {set_tvec((word_t*)vstvec, src); break;});
      set_tvec(dest, src);
      break;

    case CSR_SSCRATCH:
      IFDEF(CONFIG_RVH, if (cpu.v) {vsscratch->val = src; break;});
      sscratch->val = src;
      break;

    case CSR_SEPC:
      IFDEF(CONFIG_RVH, if(cpu.v) {vsepc->val = src & (~0x1UL); break;});
      sepc->val = src & (~0x1UL);
      break;

    case CSR_SCAUSE:
      IFDEF(CONFIG_RVH, if (cpu.v) {vscause->val = src; break;});
      scause->val = src;
      break;

    case CSR_STVAL:
      IFDEF(CONFIG_RVH, if (cpu.v) {vstval->val = src; break;});
      stval->val = src;
      break;

    case CSR_SIP:
      IFDEF(CONFIG_RVH, if (cpu.v) {vmode_set_sip(src); break;});
      non_vmode_set_sip(src);
      break;

#ifdef CONFIG_RV_SSTC
    case CSR_STIMECMP:
      IFDEF(CONFIG_RVH, if (cpu.v) {vstimecmp->val = src; break;});
      stimecmp->val = src;
      break;
#endif // CONFIG_RV_SSTC

#ifdef CONFIG_RV_IMSIC
    case CSR_SISELECT:
      IFDEF(CONFIG_RVH, if (cpu.v) {vsiselect->val = src; break;});
      siselect->val = src;
      break;
#endif // CONFIG_RV_IMSIC


    case CSR_SATP:
#ifdef CONFIG_RVH
      if (cpu.v) {
        vsatp_t new_val;
        new_val.val = src;
        // legal mode
#ifdef CONFIG_RV_SV48
        if (new_val.mode == SATP_MODE_BARE || new_val.mode == SATP_MODE_Sv39 || new_val.mode == SATP_MODE_Sv48)
#else
        if (new_val.mode == SATP_MODE_BARE || new_val.mode == SATP_MODE_Sv39)
#endif // CONFIG_RV_SV48
        {
          update_vsatp(new_val);
        }
        break;
      }
#endif // CONFIG_RVH

      // Only support Sv39 && Sv48(can configure), ignore write that sets other mode
#ifdef CONFIG_RV_SV48
      if ((src & SATP_SV39_MASK) >> 60 == 9 || (src & SATP_SV39_MASK) >> 60 == 8 || (src & SATP_SV39_MASK) >> 60 == 0)
#else
      if ((src & SATP_SV39_MASK) >> 60 == 8 || (src & SATP_SV39_MASK) >> 60 == 0)
#endif // CONFIG_RV_SV48
        *dest = MASKED_SATP(src);
      break;

#ifdef CONFIG_RV_SSCOFPMF
    case CSR_SCOUNTOVF: *dest = src & SCOUNTOVF_WMASK; break;
#endif // CONFIG_RV_SSCOFPMF

#ifdef CONFIG_RV_IMSIC
    case CSR_STOPI: return;
    case CSR_STOPEI: return;
    case CSR_SIREG:
    {
      if (cpu.v) { break; }
      if (iselect_is_major_ip(siselect->val)) {
        cpu.SIprios->iprios[(siselect->val - ISELECT_2F_MASK - 1) >> 1].val = src;
      }
      break;
    }
#endif // CONFIG_RV_IMSIC


    /************************* Hypervisor and VS CSRs *************************/
#ifdef CONFIG_RVH

    case CSR_VSSTATUS:
    {
      uint64_t vsstatus_wmask = SSTATUS_WMASK;
#ifdef CONFIG_RV_SSDBLTRP
      // when menvcfg or henvcfg.DTE close,  vsstatus.SDT is read-only
      if (menvcfg->dte == 0 || henvcfg->dte == 0) {
        src &= vsstatus_wmask & (~MSTATUS_WMASK_SDT);
      }
      // the same as mstatus SIE
      if (src & MSTATUS_SIE) {
        vsstatus_wmask &= ~MSTATUS_SIE;
        if (((src & MSTATUS_WMASK_SDT) == 0) || ( vsstatus->sdt == 0)) {
          vsstatus_wmask |= MSTATUS_SIE;
        }
      }
#endif //CONFIG_RV_SSDBLTRP
      vsstatus->val = mask_bitset(vsstatus->val, vsstatus_wmask, src);
#ifdef CONFIG_RV_SSDBLTRP
      if (src & MSTATUS_WMASK_SDT) { vsstatus->sie = 0; }
#endif //CONFIG_RV_SSDBLTRP
      break;
    }

    case CSR_VSIE: set_vsie(src); break;
    case CSR_VSTVEC: set_tvec(dest, src); break;
    case CSR_VSEPC: vsepc->val = src & (~0x1UL); break;
    case CSR_VSIP: set_vsip(src); break;
    case CSR_VSATP:
    {
      vsatp_t vsatp_new_val;
      vsatp_new_val.val = src;
      // Update vsatp without checking if vsatp.mode is legal, when hart is not in MODE_VS.
      update_vsatp(vsatp_new_val);
      break;
    }
    case CSR_HEDELEG: hedeleg->val = mask_bitset(hedeleg->val, HEDELEG_MASK, src); break;
    case CSR_HIDELEG: hideleg->val = mask_bitset(hideleg->val, HIDELEG_MASK, src); break;
    case CSR_HSTATUS: hstatus->val = mask_bitset(hstatus->val, HSTATUS_WMASK, src); break;
    case CSR_HIE: mie->val = mask_bitset(mie->val, HIE_WMASK & (mideleg->val | MIDELEG_FORCED_MASK), src); break;
    case CSR_HCOUNTEREN: hcounteren->val = mask_bitset(hcounteren->val, COUNTEREN_MASK, src); break;

#ifdef CONFIG_RV_AIA
    case CSR_HVIEN: hvien->val = mask_bitset(hvien->val, HVIEN_MSAK, src); break;
#endif // CONFIG_RV_AIA

    case CSR_HENVCFG:
      henvcfg->val = mask_bitset(henvcfg->val, HENVCFG_WMASK & (~MENVCFG_WMASK_CBIE), src);
      if ((src & MENVCFG_WMASK_CBIE) != (0x20 & MENVCFG_WMASK_CBIE)) {
        henvcfg->val = mask_bitset(henvcfg->val, MENVCFG_WMASK_CBIE, src);
      }
#ifdef CONFIG_RV_SSDBLTRP
      if(henvcfg->dte == 0) {
        vsstatus->sdt = 0;
      }
#endif // CONFIG_RV_SSDBLTRP
      break;

#ifdef CONFIG_RV_SMSTATEEN
    case CSR_HSTATEEN0:
    {
      *dest = ((src & HSTATEEN0_WMASK) | STATEEN0_CSRIND); break;
    }
#endif // CONFIG_RV_SMSTATEEN

    case CSR_HGATP:
    {
      hgatp_t hgatp_new_val;
      hgatp_new_val.val = src;
      // vmid and ppn WARL in the normal way, regardless of hgatp_new_val.mode
      hgatp->vmid = hgatp_new_val.vmid;
      // Make PPN[1:0] read only zero
      hgatp->ppn = hgatp_new_val.ppn & ~(rtlreg_t)3 & BITMASK(CONFIG_PADDRBITS - PAGE_SHIFT);

      // Only support Sv39x4 && Sv48x4(can configure), ignore write that sets other mode
#ifdef CONFIG_RV_SV48
      if (hgatp_new_val.mode == HGATP_MODE_Sv48x4 || hgatp_new_val.mode == HGATP_MODE_Sv39x4 || hgatp_new_val.mode == HGATP_MODE_BARE)
#else
      if (hgatp_new_val.mode == HGATP_MODE_Sv39x4 || hgatp_new_val.mode == HGATP_MODE_BARE)
#endif // CONFIG_RV_SV48
        hgatp->mode = hgatp_new_val.mode;
      // When MODE=Bare, software should set the remaining fields in hgatp to zeros, not hardware.
      break;
    }

    case CSR_HIP: hvip->val = mask_bitset(hvip->val, HIP_WMASK & (mideleg->val | MIDELEG_FORCED_MASK), src); break;
    case CSR_HVIP: hvip->val = mask_bitset(hvip->val, HVIP_MASK, src); break;



#ifdef CONFIG_RV_IMSIC
    case CSR_VSTOPI: return;
    case CSR_VSTOPEI: return;
    case CSR_VSIREG: return;
#endif // CONFIG_RV_IMSIC

#endif // CONFIG_RVH

    /************************* Machine-Level CSRs *************************/
    case CSR_MSTATUS:
    {
#ifdef CONFIG_RVH
      uint64_t mstatus_wmask = MSTATUS_WMASK;
      unsigned prev_mpp = mstatus->mpp;
      // only when reg.MDT is zero or wdata.MDT is zero , MIE can be explicitly written by 1
#ifdef CONFIG_RV_SMDBLTRP
      if (src & MSTATUS_MIE) {
        mstatus_wmask &= ~MSTATUS_MIE;
        if (((src & MSTATUS_WMASK_MDT) == 0) || ( mstatus->mdt == 0)) {
          mstatus_wmask |= MSTATUS_MIE;
        }
      }
#endif //CONFIG_RV_SMDBLTRP
#ifdef CONFIG_RV_SSDBLTRP
      // when menvcfg->DTE is zero, SDT field is read-only zero
      if (menvcfg->dte == 0 ) {
        src &= mstatus_wmask & (~MSTATUS_WMASK_SDT);
      }
      if (src & MSTATUS_SIE) {
        mstatus_wmask &= ~MSTATUS_SIE;
        if (((src & MSTATUS_WMASK_SDT) == 0) || ( mstatus->sdt == 0)) {
          mstatus_wmask |= MSTATUS_SIE;
        }
      }
#endif //CONFIG_RV_SSDBLTRP
      mstatus->val = mask_bitset(mstatus->val, mstatus_wmask, src);
      if (mstatus->mpp == MODE_RS) {
        // MODE_RS is reserved. write will not take effect.
        mstatus->mpp = prev_mpp;
      }
      update_mmu_state(); // maybe write update mprv, mpp or mpv
#ifdef CONFIG_RV_SMDBLTRP
      // when MDT is explicitly written by 1, clear MIE
      if (src & MSTATUS_WMASK_MDT) { mstatus->mie = 0; }
#endif // CONFIG_RV_SMDBLTRP
#ifdef CONFIG_RV_SSDBLTRP
      if (src & MSTATUS_WMASK_SDT) { mstatus->sie = 0; }
#endif // CONFIG_RV_SSDBLTRP
#else // !CONFIG_RVH
      unsigned prev_mpp = mstatus->mpp;
      mstatus->val = mask_bitset(mstatus->val, MSTATUS_WMASK, src);
      // Need to do an extra check for mstatus.MPP:
      // xPP fields are WARL fields that can hold only privilege mode x
      // and any implemented privilege mode lower than x.
      // M-mode software can determine whether a privilege mode is implemented
      // by writing that mode to MPP then reading it back. If the machine
      // provides only U and M modes, then only a single hardware storage bit
      // is required to represent either 00 or 11 in MPP.
      if (mstatus->mpp == MODE_RS) {
        // MODE_RS is reserved. The write will not take effect.
        mstatus->mpp = prev_mpp;
      }
#endif // CONFIG_RVH
      break;
    }

#ifdef CONFIG_MISA_UNCHANGEABLE
    case CSR_MISA: break;
#endif // CONFIG_MISA_UNCHANGEABLE

    case CSR_MEDELEG: medeleg->val = mask_bitset(medeleg->val, MEDELEG_MASK, src); break;
    case CSR_MIDELEG: mideleg->val = mask_bitset(mideleg->val, MIDELEG_WMASK, src); break;
    case CSR_MIE: mie->val = mask_bitset(mie->val, MIE_MASK_BASE | MIE_MASK_H | LCOFI, src); break;
    case CSR_MTVEC: set_tvec(dest, src); break;
    case CSR_MCOUNTEREN: mcounteren->val = mask_bitset(mcounteren->val, COUNTEREN_MASK, src); break;

#ifdef CONFIG_RV_AIA
    case CSR_MVIEN: mvien->val = mask_bitset(mvien->val, MVIEN_MASK, src); break;
    case CSR_MVIP: set_mvip(src); break;
#endif // CONFIG_RV_AIA

    case CSR_MENVCFG:
      menvcfg->val = mask_bitset(menvcfg->val, MENVCFG_WMASK & (~MENVCFG_WMASK_CBIE), src);
      if (((menvcfg_t*)&src)->cbie != 0b10) { // 0b10 is reserved
        menvcfg->val = mask_bitset(menvcfg->val, MENVCFG_WMASK_CBIE, src);
      }
#ifdef CONFIG_RV_SSDBLTRP
      if(menvcfg->dte == 0) {
        mstatus->sdt = 0;
        vsstatus->sdt = 0;
      }
#endif // CONFIG_RV_SSDBLTRP
      break;

#ifdef CONFIG_RV_SMSTATEEN
    case CSR_MSTATEEN0: *dest = ((src & MSTATEEN0_WMASK) | STATEEN0_CSRIND); break;
#endif // CONFIG_RV_SMSTATEEN

#ifdef CONFIG_RV_CSR_MCOUNTINHIBIT
    case CSR_MCOUNTINHIBIT:
      update_counter_mcountinhibit(mcountinhibit->val, src & MCOUNTINHIBIT_MASK);
      mcountinhibit->val = mask_bitset(mcountinhibit->val, MCOUNTINHIBIT_MASK, src);
      break;
#endif // CONFIG_RV_CSR_MCOUNTINHIBIT

    case CSR_MHPMEVENT_BASE ... CSR_MHPMEVENT_BASE+CSR_MHPMEVENT_NUM-1:
    {
      mhpmevent3_t *mhpmevent = (mhpmevent3_t *)dest;
      unsigned pre_op0 = mhpmevent->optype0;
      unsigned pre_op1 = mhpmevent->optype1;
      unsigned pre_op2 = mhpmevent->optype2;
      mhpmevent3_t new_val;
      new_val.val = src;
      *dest = src & MHPMEVENT_WMASK;
      if (!hpmevent_op_islegal(new_val.optype0)) {
        mhpmevent->optype0 = pre_op0;
      }
      if (!hpmevent_op_islegal(new_val.optype1)) {
        mhpmevent->optype1 = pre_op1;
      }
      if (!hpmevent_op_islegal(new_val.optype2)) {
        mhpmevent->optype2 = pre_op2;
      }
      break;
    }

    case CSR_MEPC: *dest = src & (~0x1UL); break;
    case CSR_MIP: set_mip(src); break;

#ifdef CONFIG_RV_PMP_CSR
    case CSR_PMPCFG_BASE ... CSR_PMPCFG_BASE+CSR_PMPCFG_MAX_NUM-1:
    {
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
      break;
    }

    case CSR_PMPADDR_BASE ... CSR_PMPADDR_BASE+CSR_PMPADDR_MAX_NUM-1:
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
      break;

#endif // CONFIG_RV_PMP_CSR

#ifdef CONFIG_RV_SMRNMI
    case CSR_MNEPC: *dest = src & (~0x1UL); break;
    case CSR_MNSTATUS:
    {
      word_t mnstatus_mask = MNSTATUS_MASK;
      unsigned pre_mnpp = mnstatus->mnpp;
// as opensbi and linux not support smrnmi, so we default init nmie = 1 and allow nmie set to 0 by software for test
      if ((src & MNSTATUS_NMIE) == 0 && !ISDEF(CONFIG_NMIE_INIT)) {
        mnstatus_mask &= ~MNSTATUS_NMIE;
      }
      mnstatus->val = mask_bitset(mnstatus->val, mnstatus_mask, src);
      if (mnstatus->mnpp == MODE_RS) {
        mnstatus->mnpp = pre_mnpp;
      }
      break;
    }
#endif //CONFIG_RV_SMRNMI

#ifdef CONFIG_RV_SDTRIG
    case CSR_TSELECT:
      *dest = src < CONFIG_TRIGGER_NUM ? src : tselect->val;
      break;
    case CSR_TDATA1:
    {
      // not write to dest
      tdata1_t* tdata1_reg = &cpu.TM->triggers[tselect->val].tdata1.common;
      tdata1_t tdata1_wdata = *(tdata1_t*)&src;
      switch (tdata1_wdata.type)
      {
      case TRIG_TYPE_NONE: // write type 0 to disable this trigger
      case TRIG_TYPE_DISABLE:
        tdata1_reg->type = TRIG_TYPE_DISABLE;
        tdata1_reg->data = 0;
        break;
      case TRIG_TYPE_ICOUNT:
        icount_checked_write(&cpu.TM->triggers[tselect->val].tdata1.icount, &src);
        break;
      case TRIG_TYPE_ITRIG:
        itrigger_checked_write(&cpu.TM->triggers[tselect->val].tdata1.itrigger, &src);
        break;
      case TRIG_TYPE_ETRIG:
        etrigger_checked_write(&cpu.TM->triggers[tselect->val].tdata1.etrigger, &src);
        break;
      case TRIG_TYPE_MCONTROL6:
        mcontrol6_checked_write(&cpu.TM->triggers[tselect->val].tdata1.mcontrol6, &src, cpu.TM);
        break;
      default:
        // do nothing for not supported trigger type
        break;
      }
      break;
    }
    case CSR_TDATA2:
    {
      // not write to dest
      tdata2_t* tdata2_reg = &cpu.TM->triggers[tselect->val].tdata2;
      tdata2_t tdata2_wdata = *(tdata2_t*)&src;
      tdata2_reg->val = tdata2_wdata.val;
      break;
    }
#ifdef CONFIG_SDTRIG_EXTRA
    case CSR_TDATA3:
    {
      tdata3_t* tdata3_reg = &cpu.TM->triggers[tselect->val].tdata3;
      tdata3_t tdata3_wdata = *(tdata3_t*)&src;
      tdata3_reg->val = tdata3_wdata.val;
      break;
    }
#endif // CONFIG_SDTRIG_EXTRA
#endif // CONFIG_RV_SDTRIG

    case CSR_MCYCLE:  mcycle->val = set_mcycle(src); break;
    case CSR_MINSTRET: minstret->val = set_minstret(src); break;

    case CSR_MHPMCOUNTER_BASE ... CSR_MHPMCOUNTER_BASE+CSR_MHPMCOUNTER_NUM-1: break;

#ifdef CONFIG_RV_IMSIC
    case CSR_MTOPI: return;
    case CSR_MTOPEI: return;
    case CSR_MIREG:
    {
      if (iselect_is_major_ip(miselect->val)) {
        cpu.MIprios->iprios[(miselect->val - ISELECT_2F_MASK - 1) >> 1].val = src;
      }
      break;
    }
#endif // CONFIG_RV_IMSIC

    /************************* All Others Normal CSRs *************************/
    default: *dest = src;
  }

 // Next is the side effect of writing CSRs
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
  if (is_write(mstatus) || is_write(satp) || is_write(vsatp)
      || is_write(hgatp) || MUXDEF(CONFIG_RV_SMRNMI, is_write(mnstatus), false)) { update_mmu_state(); }
  if (is_write(hstatus)) {
    set_sys_state_flag(SYS_STATE_FLUSH_TCACHE); // maybe change virtualization mode
  }
#else
  if (is_write(mstatus) || is_write(satp) || MUXDEF(CONFIG_RV_SMRNMI, is_write(mnstatus), false)) { update_mmu_state(); }
#endif
  if (is_write(satp)) { mmu_tlb_flush(0); } // when satp is changed(asid | ppn), flush tlb.
  if (is_write(mstatus) || is_write(sstatus) || is_write(satp) ||
      is_write(mie) || is_write(sie) || is_write(mip) || is_write(sip)) {
    set_sys_state_flag(SYS_STATE_UPDATE);
  }

#ifdef CONFIG_RV_IMSIC
  if (is_write(mideleg) || is_write(hideleg) || is_write(hstatus) || is_write(hvictl) ||
      is_write(mip) || is_write(mvip) || is_write(hvip) || is_write(hip) || is_write(sip) || is_write(vsip) ||
      is_write(mie) || is_write(mvien) || is_write(hvien) || is_write(hie) || is_write(sie) || is_write(vsie) ||
      is_write(mireg) || is_write(sireg)) {
    update_mtopi();
    update_stopi();
    update_vstopi();
  }
#endif
}

static inline bool satp_permit_check(const word_t *dest_access){
  bool has_vi = false;
  if (is_access(satp)){
    #ifdef CONFIG_RVH
    // HS access satp when mstatus.tvm = 1 will cause EX_II
    if (!cpu.v && cpu.mode == MODE_S && mstatus->tvm) {
      longjmp_exception(EX_II);
    }
    // VS access satp when hstatus.vtvm = 1 will cause EX_VI
    if (cpu.v && cpu.mode == MODE_S && hstatus->vtvm) {
      has_vi = true;
    }
    #else // CONFIG_RVH
    // HS access satp when mstatus.tvm = 1 will cause EX_II
    if (cpu.mode == MODE_S && mstatus->tvm) {
      longjmp_exception(EX_II);
    }
    #endif // CONFIG_RVH
  }
  #ifdef CONFIG_RVH
  else if (is_access(hgatp)) {
    // HS access hgatp when mstatus.tvm = 1 will cause EX_II
    if(!cpu.v && cpu.mode == MODE_S && mstatus->tvm) {
      longjmp_exception(EX_II);
    }
  }
  #endif // CONFIG_RVH
  return has_vi;
}

// VS/VU access stateen should be EX_II when mstateen0->se0 is false.
#ifdef CONFIG_RV_SMSTATEEN
static inline bool smstateen_extension_permit_check(const word_t *dest_access) {
  bool has_vi = false;
  if (is_access(sstateen0)) {
    if ((cpu.mode < MODE_M) && (!mstateen0->se0)) { longjmp_exception(EX_II); }
#ifdef CONFIG_RVH
    else if (cpu.v && mstateen0->se0 && !hstateen0->se0) { has_vi = true; }
#endif // CONFIG_RVH
  }
#ifdef CONFIG_RVH
  else if (is_access(hstateen0)) {
    if ((cpu.mode < MODE_M) && (!mstateen0->se0)) { longjmp_exception(EX_II); }
    else if (cpu.v && mstateen0->se0) { has_vi = true;}
  }
#endif // CONFIG_RVH
  return has_vi;
}
#endif // CONFIG_RV_SMSTATEEN

// AIA extension check
// !!! Only support in RVH
#ifdef CONFIG_RV_IMSIC
static bool aia_extension_permit_check(const word_t *dest_access, bool is_write) {
  bool has_vi = false;
  if (is_access(stopei)) {
    if (!cpu.v && (cpu.mode == MODE_S) && mvien->seie) {
      longjmp_exception(EX_II);
    }
  }
  if (is_access(mireg)) {
    if (
      (miselect->val <= ISELECT_2F_MASK) ||
      (miselect->val > ISELECT_2F_MASK && miselect->val <= ISELECT_3F_MASK && miselect->val & 0x1) ||
      (miselect->val > ISELECT_3F_MASK && miselect->val <= ISELECT_6F_MASK) || 
      (miselect->val > ISELECT_7F_MASK && miselect->val <= ISELECT_MAX_MASK && miselect->val & 0x1) ||
      (miselect->val > ISELECT_MAX_MASK)
    ) {
      longjmp_exception(EX_II);
    }
  }
  if (is_access(sireg)) {
    if (!cpu.v) {
      if (
        (siselect->val <= ISELECT_2F_MASK) ||
        (siselect->val > ISELECT_2F_MASK && siselect->val <= ISELECT_3F_MASK && siselect->val & 0x1) ||
        (siselect->val > ISELECT_3F_MASK && siselect->val <= ISELECT_6F_MASK) ||
        (cpu.mode == MODE_S && mvien->seie && siselect->val > ISELECT_6F_MASK && siselect->val <= ISELECT_MAX_MASK) ||
        (siselect->val > ISELECT_7F_MASK && siselect->val <= ISELECT_MAX_MASK && siselect->val & 0x1) ||
        (siselect->val > ISELECT_MAX_MASK)
      ) {
        longjmp_exception(EX_II);
      }
    }
    if (cpu.v) {
      if (
        (vsiselect->val <= ISELECT_2F_MASK) ||
        (vsiselect->val > ISELECT_3F_MASK && vsiselect->val <= ISELECT_6F_MASK) ||
        (vsiselect->val > ISELECT_MAX_MASK)
      ) {
        longjmp_exception(EX_II);
      }
      if (
        (vsiselect->val > ISELECT_2F_MASK && vsiselect->val <= ISELECT_3F_MASK) ||
        ((hstatus->vgein == 0 || hstatus->vgein > CONFIG_GEILEN) && vsiselect->val > ISELECT_6F_MASK && vsiselect->val <= ISELECT_MAX_MASK) ||
        (vsiselect->val > ISELECT_7F_MASK && vsiselect->val <= ISELECT_MAX_MASK && vsiselect->val & 0x1)
      ) {
        has_vi = true;
      }
    }
  }
  if (is_access(vsireg)) {
    if (
      (vsiselect->val <= ISELECT_6F_MASK) ||
      ((hstatus->vgein == 0 || hstatus->vgein > CONFIG_GEILEN) && vsiselect->val > ISELECT_6F_MASK && vsiselect->val <= ISELECT_MAX_MASK) ||
      (vsiselect->val > ISELECT_7F_MASK && vsiselect->val <= ISELECT_MAX_MASK && vsiselect->val & 0x1) ||
      (vsiselect->val > ISELECT_MAX_MASK)
    ) {
      longjmp_exception(EX_II);
    }
  }
  if (is_access(sip) || is_access(sie)) {
    if (cpu.v && (cpu.mode == MODE_S)) {
      if (hvictl->vti) {
        has_vi = true;
      }
    }
  }
  if (is_access(stimecmp)) {
    if (cpu.v && (cpu.mode == MODE_S) && hvictl->vti && is_write) {
      has_vi = 1;
    }
  }
  return has_vi;
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
static inline bool fp_permit_check(const word_t *dest_access) {
  if (is_access(fcsr) || is_access(fflags) || is_access(frm)) {
    if (!require_fs()) { longjmp_exception(EX_II); }
  }
  return false;
}
#endif // CONFIG_FPU_NONE

#ifdef CONFIG_RVV
static inline bool vec_permit_check(const word_t *dest_access) {
  if (is_access(vcsr) || is_access(vlenb) || is_access(vstart) || is_access(vxsat) || is_access(vxrm) || is_access(vl) || is_access(vtype)) {
    if (!require_vs()) { longjmp_exception(EX_II); }
  }
  return false;
}
#endif // CONFIG_RVV

static inline void csr_permit_check(uint32_t addr, bool is_write) {
  bool has_vi = false; // virtual instruction
  word_t *dest_access = csr_decode(addr);
  // check csr_exit, priv
  has_vi |= csr_normal_permit_check(addr);

  // check csr_readonly
  has_vi |= csr_readonly_permit_check(addr, is_write);

  // Attempts to access unprivileged counters without s/h/mcounteren
  if ((addr >= 0xC00 && addr <= 0xC1F) || (addr == 0x14D) || (addr == 0x24D)) {
    has_vi |= csr_counter_enable_check(addr);
  }
  // check smstateen
  IFDEF(CONFIG_RV_SMSTATEEN, has_vi |= smstateen_extension_permit_check(dest_access));

  // check aia
  IFDEF(CONFIG_RV_IMSIC, has_vi |= aia_extension_permit_check(dest_access, is_write));

  //check satp(satp & hgatp)
  has_vi |= satp_permit_check(dest_access);

  //check fp
  IFNDEF(CONFIG_FPU_NONE, has_vi |= fp_permit_check(dest_access));
  //check vec
  IFDEF(CONFIG_RVV, has_vi |= vec_permit_check(dest_access));

  if (has_vi) longjmp_exception(EX_VI);

}
static void csrrw(rtlreg_t *dest, const rtlreg_t *src, uint32_t csrid, uint32_t instr) {
  ISADecodeInfo isa;
  isa.instr.val = instr;
  uint32_t rs1    = isa.instr.i.rs1; // uimm field and rs1 field are the same one
  uint32_t rd     = isa.instr.i.rd;
  uint32_t funct3 = isa.instr.i.funct3;
  bool is_write = !( BITS(funct3, 1, 1) && (rs1 == 0) );
  csr_permit_check(csrid, is_write);
  switch (funct3) {
    case FUNCT3_CSRRW:
    case FUNCT3_CSRRWI:
      if (rd) {
        *dest = csr_read(csrid);
      }
      csr_write(csrid, *src);
      break;
    case FUNCT3_CSRRS:
    case FUNCT3_CSRRSI:
      *dest = csr_read(csrid);
      if (rs1) {
        csr_write(csrid, *src | *dest);
      }
      break;
    case FUNCT3_CSRRC:
    case FUNCT3_CSRRCI:
      *dest = csr_read(csrid);
      if (rs1) {
        csr_write(csrid, (~*src) & *dest);
      }
      break;
    default: panic("funct3 = %d is not supported for csrrw instruction\n", funct3);
  }
}

static bool execIn (cpu_mode_t mode) {
  switch (mode) {
    case CPU_MODE_M:  
      return cpu.mode == MODE_M;
    case CPU_MODE_S:  
      return cpu.mode == MODE_S && MUXDEF(CONFIG_RVH, !cpu.v, 1); 
  #ifdef CONFIG_RVH
    case CPU_MODE_VS: 
      return cpu.mode == MODE_S && cpu.v;
    case CPU_MODE_VU:
      return cpu.mode == MODE_U && cpu.v;
  #endif
    case CPU_MODE_U:
      return cpu.mode == MODE_U && MUXDEF(CONFIG_RVH, !cpu.v, 1);
    default:
      assert(0);
  }
}

static bool mretTo (cpu_mode_t mode) {
  switch (mode) {
    case CPU_MODE_M:
      return mstatus->mpp == MODE_M;
    case CPU_MODE_S:
      return mstatus->mpp == MODE_S && MUXDEF(CONFIG_RVH, !mstatus->mpv, 1);
  #ifdef CONFIG_RVH
    case CPU_MODE_VS: 
      return mstatus->mpp == MODE_S && mstatus->mpv;
    case CPU_MODE_VU:
      return mstatus->mpp == MODE_U && mstatus->mpv;
  #endif  
    case CPU_MODE_U:
      return mstatus->mpp == MODE_U && MUXDEF(CONFIG_RVH, !mstatus->mpv, 1);
    default:
      assert(0);
  }
}

#ifdef CONFIG_RV_SMRNMI
static bool mnretTo (cpu_mode_t mode) {
  switch (mode) {
    case CPU_MODE_M:
      return mnstatus->mnpp == MODE_M;
    case CPU_MODE_S:
      return mnstatus->mnpp == MODE_S && MUXDEF(CONFIG_RVH, !mnstatus->mnpv, 1);
  #ifdef CONFIG_RVH
    case CPU_MODE_VS: 
      return mnstatus->mnpp == MODE_S && mnstatus->mnpv;
    case CPU_MODE_VU:
      return mnstatus->mnpp == MODE_U && mnstatus->mnpv;
  #endif  
    case CPU_MODE_U:
      return mnstatus->mnpp == MODE_U &&  MUXDEF(CONFIG_RVH, !mnstatus->mnpv, 1);
    default:
      assert(0);
  }
}
#endif

static bool sretTo (cpu_mode_t mode) {
  assert(execIn(CPU_MODE_S) || execIn(CPU_MODE_M));
  switch (mode) {
    case CPU_MODE_S:
      return mstatus->spp == MODE_S && MUXDEF(CONFIG_RVH, !hstatus->spv, 1);
  #ifdef CONFIG_RVH
    case CPU_MODE_VS: 
      return mstatus->spp == MODE_S && hstatus->spv;
    case CPU_MODE_VU:
      return mstatus->spp == MODE_U && hstatus->spv;
  #endif  
    case CPU_MODE_U:
      return mstatus->spp == MODE_U && MUXDEF(CONFIG_RVH, !hstatus->spv, 1);
    default:
      assert(0);
  }
}

// exec sret in VS
#ifdef CONFIG_RVH
static bool __attribute__((unused)) vsretTo (cpu_mode_t mode) {
  switch (mode) {
    case CPU_MODE_VS:
      return vsstatus->spp == MODE_S;
    case CPU_MODE_VU:
      return vsstatus->spp == MODE_U;
    default:
      assert(0);
  }
}
#endif

static word_t priv_instr(uint32_t op, const rtlreg_t *src) {
  switch (op) {
#ifndef CONFIG_MODE_USER
    case 0x102: // sret
#ifdef CONFIG_RVH
      if (cpu.v == 1){
        if((cpu.mode == MODE_S && hstatus->vtsr) || cpu.mode < MODE_S){
          longjmp_exception(EX_VI);
        }
        if (ISDEF(CONFIG_RV_SSDBLTRP)) {
          vsstatus->sdt = 0;
        }
        cpu.mode = vsstatus->spp;
        vsstatus->spp  = MODE_U;
        vsstatus->sie  = vsstatus->spie;
        vsstatus->spie = 1;
        return vsepc->val;
      }
#endif // CONFIG_RVH
      // cpu.v = 0
      if ((cpu.mode == MODE_S && mstatus->tsr) || cpu.mode < MODE_S) {
        longjmp_exception(EX_II);
      }
      if (execIn(CPU_MODE_M) && ISDEF(CONFIG_RV_SMDBLTRP)) {
        if (ISDEF(CONFIG_RV_SSDBLTRP) && (sretTo(CPU_MODE_VU) || sretTo(CPU_MODE_VS) || sretTo(CPU_MODE_U))) {
          mstatus->sdt = 0;
        }
        if (ISDEF(CONFIG_RV_SSDBLTRP) && sretTo(CPU_MODE_VU)) {
          IFDEF(CONFIG_RVH,vsstatus->sdt = 0;)
        }
        mstatus->mdt = 0;
      } else if (execIn(CPU_MODE_S) && ISDEF(CONFIG_RV_SSDBLTRP) ) {
        mstatus->sdt = 0;
        if (sretTo(CPU_MODE_VU)) {
          IFDEF(CONFIG_RVH,vsstatus->sdt = 0;)
        }
      }
#ifdef CONFIG_RVH
      cpu.v = hstatus->spv;
      hstatus->spv = 0;
      set_sys_state_flag(SYS_STATE_FLUSH_TCACHE);
#endif //CONFIG_RVH
      mstatus->sie = mstatus->spie;
      mstatus->spie = (ISDEF(CONFIG_DIFFTEST_REF_QEMU) ? 0 // this is bug of QEMU
          : 1);
      if (mstatus->spp != MODE_M) { mstatus->mprv = 0; }
      cpu.mode = mstatus->spp;
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
      if (execIn(CPU_MODE_M)) {
        if (ISDEF(CONFIG_RV_SMDBLTRP) || ISDEF(CONFIG_RV_SSDBLTRP)) {
          if(mretTo(CPU_MODE_U) || mretTo(CPU_MODE_VU) || mretTo(CPU_MODE_VS)) {
            mstatus->sdt = 0;
          }
          if (mretTo(CPU_MODE_VU)) {
            IFDEF(CONFIG_RVH,vsstatus->sdt = 0;)
          }
        }
        if (ISDEF(CONFIG_RV_SMDBLTRP)) {
          mstatus->mdt = 0;
        }
      }
#ifdef CONFIG_RVH
      cpu.v = (mstatus->mpp == MODE_M ? 0 : mstatus->mpv);
      mstatus->mpv = 0;
      set_sys_state_flag(SYS_STATE_FLUSH_TCACHE);
#endif // CONFIG_RVH
      if (mstatus->mpp != MODE_M) { mstatus->mprv = 0; }
      cpu.mode = mstatus->mpp;
      mstatus->mpp = MODE_U;
      update_mmu_state();
      Loge("Executing mret to 0x%lx", mepc->val);
      return mepc->val;
      break;
#ifdef CONFIG_RV_SMRNMI
    case 0x702: // mnret
      if (cpu.mode < MODE_M) {
        longjmp_exception(EX_II);
      }
      if (mnstatus->mnpp != MODE_M) { mstatus->mprv = 0; }
#ifdef CONFIG_RVH
      cpu.v    = (mnstatus->mnpp == MODE_M ? 0 : mnstatus->mnpv);
      mnstatus->mnpv = 0;
      // clear vsstatus.SDT when return to VU
      vsstatus->sdt = (mnstatus->mnpp == MODE_U && mnstatus->mnpv == 1 ? 0 : vsstatus->sdt);
      set_sys_state_flag(SYS_STATE_FLUSH_TCACHE);
#endif // config_RVH
      // clear MDT when mnret to below M
      if (ISDEF(CONFIG_RV_SMDBLTRP)) {
        if (!mnretTo(CPU_MODE_M)) {
          mstatus->mdt = 0;
        }
        if (ISDEF(CONFIG_RV_SSDBLTRP)) {
          if (mnretTo(CPU_MODE_U) || mnretTo(CPU_MODE_VU) || mnretTo(CPU_MODE_VS)) {
            mstatus->sdt = 0;
          }
          if (mnretTo(CPU_MODE_VU)) {
            IFDEF(CONFIG_RVH,vsstatus->sdt = 0;)
          }
        }
      }
      cpu.mode = mnstatus->mnpp;
      mnstatus->mnpp = MODE_U;
      mnstatus->nmie = 1; 
      update_mmu_state();
      Loge("Executing mnret to 0x%lx", mnepc->val);
      return mnepc->val;
      break;

#endif //CONFIG_RV_SMRNMI
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
    case (uint32_t)-1: // fence.i
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
    case HOSTCALL_CSR: csrrw(dest, src1, *src2, imm); return;
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
  int mmu_mode = get_hyperinst_mmu_state();
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
  int mmu_mode = get_hyperinst_mmu_state();
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
