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

#include <isa.h>
#include <memory/vaddr.h>
#include <memory/paddr.h>
#include <memory/host.h>
#include <cpu/cpu.h>
#include "../local-include/csr.h"
#include "../local-include/intr.h"
#include "../local-include/rtl.h"

typedef union PageTableEntry {
  struct {
    uint32_t v   : 1;
    uint32_t r   : 1;
    uint32_t w   : 1;
    uint32_t x   : 1;
    uint32_t u   : 1;
    uint32_t g   : 1;
    uint32_t a   : 1;
    uint32_t d   : 1;
    uint32_t rsw : 2;
    uint64_t ppn :44;
    uint32_t pad : 7;
    uint32_t pbmt: 2;
    uint32_t n   : 1;
  };
  uint64_t val;
} PTE;

#define PGSHFT 12
#ifdef CONFIG_RV_MBMC
#define BMSHFT 32
#endif
#define PGMASK ((1ull << PGSHFT) - 1)
#define PGBASE(pn) ((uint64_t) pn << PGSHFT)
#ifdef CONFIG_RV_MBMC
#define BMBASE(bma) (bma << BMSHFT)
#define GET_BIT(bm_base, ppn) (((bm_base[(ppn) / 8] >> ((ppn) % 8)) & 1))
static int pt_level = 0;
#endif

// Sv39 & Sv48 page walk
#define PTE_SIZE 8
#define VPNMASK 0x1ff
#define GPVPNMASK 0x7ff
#define SVNAPOTMASK 0b1111 // only suppoprt 64 KiB contiguous region
#define SVNAPOTSHFT ((PGSHFT) + 4)
static inline uintptr_t VPNiSHFT(int i) {
  return (PGSHFT) + 9 * i;
}
static inline uintptr_t VPNi(vaddr_t va, int i) {
  return (va >> VPNiSHFT(i)) & VPNMASK;
}
#ifdef CONFIG_RVH
static inline uintptr_t GVPNi(vaddr_t va, int i, int max_level) {
  return (i == max_level - 1)?  (va >> VPNiSHFT(i)) & GPVPNMASK : (va >> VPNiSHFT(i)) & VPNMASK;
}
bool hlvx = 0;
bool hld_st = 0;
#endif
#ifdef CONFIG_RVH
static inline bool check_permission(PTE *pte, bool ok, vaddr_t vaddr, int type, int virt, int mode) {
bool ifetch = (type == MEM_TYPE_IFETCH);
#else
static inline bool check_permission(PTE *pte, bool ok, vaddr_t vaddr, int type) {
  bool ifetch = (type == MEM_TYPE_IFETCH);
  uint32_t mode = (mstatus->mprv && !ifetch ? mstatus->mpp : cpu.mode);
#endif
  assert(mode == MODE_U || mode == MODE_S);
  ok = ok && pte->v;
  ok = ok && !(mode == MODE_U && !pte->u);
  ok = ok && (!pte->n || (pte->ppn & SVNAPOTMASK) == 0b1000);
#ifdef CONFIG_RVH
  ok = ok && !(pte->u && ((mode == MODE_S) && (!(virt? vsstatus->sum: mstatus->sum) || ifetch)));
  Logtr("ok: %i, mode == U: %i, pte->u: %i, ppn: %lx, virt: %d", ok, mode == MODE_U, pte->u, (uint64_t)pte->ppn << 12, virt);
#else
  ok = ok && !(pte->u && ((mode == MODE_S) && (!mstatus->sum || ifetch)));
  Logtr("ok: %i, mode: %s, pte->u: %i, a: %i d: %i, ppn: %lx ", ok,
        mode == MODE_U ? "U" : MODE_S ? "S" : MODE_M ? "M" : "NOTYPE",
        pte->u, pte->a, pte->d, (uint64_t)pte->ppn << 12);
#endif
  if (ifetch) {
    Logtr("Translate for instr reading");
#ifdef CONFIG_SHARE
//  update a/d by exception
    bool update_ad = !pte->a;
    if (update_ad && ok && pte->x)
      Logtr("raise exception to update ad for ifecth");
#else
    bool update_ad = false;
#endif
    if (!(ok && pte->x && !pte->pad) || update_ad) {
      assert(!cpu.amo);
      cpu.trapInfo.tval = vaddr;
      longjmp_exception(EX_IPF);
      return false;
    }
  } else if (type == MEM_TYPE_READ) {
    Logtr("Translate for memory reading");
#ifdef CONFIG_RVH
  bool can_load;
  if(hlvx)
    can_load = pte->x;
  else
    can_load = pte->r || ((mstatus->mxr || (vsstatus->mxr && virt)) && pte->x);
#else
  bool can_load = pte->r || (mstatus->mxr && pte->x);
#endif
#ifdef CONFIG_SHARE
    bool update_ad = !pte->a;
    if (update_ad && ok && can_load)
      Logtr("raise exception to update ad for load");
#else
    bool update_ad = false;
#endif
    if (!(ok && can_load && !pte->pad) || update_ad) {
      if (cpu.amo) Logtr("redirect to AMO page fault exception at pc = " FMT_WORD, cpu.pc);
      int ex = (cpu.amo ? EX_SPF : EX_LPF);
      cpu.trapInfo.tval = vaddr;
      cpu.amo = false;
      Logtr("Memory read translation exception!");
      longjmp_exception(ex);
      return false;
    }
  } else { // MEM_TYPE_WRITE
#ifdef CONFIG_SHARE
    bool update_ad = !pte->a || !pte->d;
   if (update_ad && ok && pte->w) Logtr("raise exception to update ad for store");
#else
    bool update_ad = false;
#endif
    Logtr("Translate for memory writing v: %d w: %d", pte->v, pte->w);
    if (!(ok && pte->w && !pte->pad) || update_ad) {
      cpu.trapInfo.tval = vaddr;
      cpu.amo = false;
      longjmp_exception(EX_SPF);
      return false;
    }
  }
  return true;
}

#ifndef CONFIG_RVH
vaddr_t get_effective_address(vaddr_t vaddr, int type) {
  return vaddr;
}
#endif

#ifdef CONFIG_RVH
bool has_two_stage_translation(){
  return hld_st || (mstatus->mprv && mstatus->mpv) || cpu.v;
}

void raise_guest_excep(paddr_t gpaddr, vaddr_t vaddr, int type, bool is_support_vs) {
  // printf("gpaddr: " FMT_PADDR ", vaddr: " FMT_WORD "\n", gpaddr, vaddr);
#ifdef FORCE_RAISE_PF
  if (
    cpu.guided_exec && cpu.execution_guide.force_raise_exception &&
    (cpu.execution_guide.exception_num == EX_IPF ||
    cpu.execution_guide.exception_num == EX_LPF ||
    cpu.execution_guide.exception_num == EX_SPF)
  ) {
    force_raise_pf(vaddr, type);
  }
#endif // FORCE_RAISE_PF
  uint64_t tinst = 0;
  tinst |= is_support_vs ? 0x3000 : 0;
  int ex = EX_LGPF;
  if (type == MEM_TYPE_IFETCH) {
    ex = EX_IGPF;
  } else if (type == MEM_TYPE_WRITE || cpu.amo) {
    ex = EX_SGPF;
  } else {
    ex = EX_LGPF;
  }
  cpu.trapInfo.tval  = vaddr;
  cpu.trapInfo.tval2 = gpaddr >> 2;
  cpu.trapInfo.tinst = tinst;
  longjmp_exception(ex);
}

vaddr_t get_effective_address(vaddr_t vaddr, int type) {
  if (type == MEM_TYPE_IFETCH || hlvx) {
    return vaddr;
  }

  bool virt = cpu.v;
  int mode = cpu.mode;
  int pmm = 0;
  int masked_width = 0;

  // Early out fastpath for non-H & non-pmm applications
  if (likely(!hld_st && !mstatus->mprv && mode == MODE_U && senvcfg->pmm == 0)) {
    return vaddr;
  }

  if (hld_st) {
    mode = hstatus->spvp;
    virt = true;
  } else if (mstatus->mprv) {
    mode = mstatus->mpp;
    virt = mstatus->mpv && mode != MODE_M;
  }

  if (mode == MODE_M) {
    pmm = mseccfg->pmm;
  } else if (!virt && mode == MODE_S) {
    pmm = menvcfg->pmm;
  } else if (virt && mode == MODE_S) {
    pmm = henvcfg->pmm;
    // Is cpu.mode here
  } else if (hld_st && cpu.mode == MODE_U) {
    pmm = hstatus->hupmm;
  } else if (mode == MODE_U) {
    pmm = senvcfg->pmm;
  } else {
    assert(0);
  }

  switch (pmm) {
    case 2:
      masked_width = 7;
      break;
    case 3:
      masked_width = 16;
      break;
  }

  bool isBare = mode == MODE_M;
  bool isPaddr = !virt && satp->mode == SATP_MODE_BARE;
  bool isGpaddr = virt && vsatp->mode == SATP_MODE_BARE;

  if (isBare || isPaddr || isGpaddr) {
    return ((uint64_t)vaddr << masked_width) >> masked_width;
  } else {
    return ((int64_t)vaddr << masked_width) >> masked_width;
  }
}

paddr_t gpa_stage(paddr_t gpaddr, vaddr_t vaddr, int type, int trap_type, bool ishlvx, bool is_support_vs){
  Logtr("gpa_stage gpaddr: " FMT_PADDR ", vaddr: " FMT_WORD ", type: %d", gpaddr, vaddr, type);
  int max_level = 0;
  #ifdef CONFIG_RV_MBMC
  pt_level = 0;
  #endif
  if (hgatp->mode == HGATP_MODE_BARE) {
    return gpaddr;
  } else if (hgatp->mode == HGATP_MODE_Sv48x4){
    if((gpaddr & ~(((int64_t)1 << 50) - 1)) != 0){
      raise_guest_excep(gpaddr, vaddr, trap_type, is_support_vs);
    }
    max_level = 4;
  } else if (hgatp->mode == HGATP_MODE_Sv39x4){
    if((gpaddr & ~(((int64_t)1 << 41) - 1)) != 0){
      raise_guest_excep(gpaddr, vaddr, trap_type, is_support_vs);
    }
    max_level = 3;
  }
  bool pbmte = menvcfg->pbmte;
  word_t pg_base = PGBASE(hgatp->ppn);
  int level;
  word_t p_pte;
  PTE pte;
  for (level = max_level - 1; level >= 0; ) {
    p_pte = pg_base + GVPNi(gpaddr, level, max_level) * PTE_SIZE;
#ifdef CONFIG_MULTICORE_DIFF
    pte.val = golden_pmem_read(p_pte, PTE_SIZE);
#else
    pte.val	= paddr_read(p_pte, PTE_SIZE,
    type == MEM_TYPE_IFETCH ? MEM_TYPE_IFETCH_READ :
    type == MEM_TYPE_WRITE ? MEM_TYPE_WRITE_READ : MEM_TYPE_READ, trap_type, MODE_S, vaddr);
#endif

    #ifdef CONFIG_SHARE
        if (unlikely(dynamic_config.debug_difftest)) {
          fprintf(stderr, "[NEMU] ptw g stage: level %d, vaddr 0x%lx, gpaddr 0x%lx, pg_base 0x%lx, p_pte 0x%lx, pte.val 0x%lx\n",
            level, vaddr, gpaddr, pg_base, p_pte, pte.val);
        }
    #endif
    pg_base = PGBASE(pte.ppn);
    Logtr(
      "g p_pte: %lx pg base:0x%lx, v:%d, r:%d, w:%d, x:%d, u:%d, g:%d, a:%d, d:%d",
      p_pte, pg_base, pte.v, pte.r, pte.w, pte.x, pte.u, pte.g, pte.a, pte.d
    );
    if (pte.v && !pte.r && !pte.w && !pte.x) {
      // not leaf
      if (pte.a || pte.d || pte.u || pte.pbmt || pte.n) {
        break;
      }
      level --;
      if (level < 0) { break; }
    } else if (!pte.v || (!pte.r && pte.w) || pte.pad) {
      break;
    } else if ((ISNDEF(CONFIG_RV_SVPBMT) || !pbmte) && pte.pbmt) {
      break;
    } else if (pte.pbmt == 3) {
      break;
    } else if (ISNDEF(CONFIG_RV_SVNAPOT) && pte.n) {
      break;
    } else if (!pte.u) {
      break;
    } else if (
      type == MEM_TYPE_IFETCH || ishlvx ? !pte.x:
      type == MEM_TYPE_READ           ? !pte.r && !(mstatus->mxr && pte.x):
                                        !(pte.r && pte.w)
    ) {
      break;
    } else if (!pte.a || (!pte.d && type == MEM_TYPE_WRITE)) {
      // TODO: support hardware a/d update.
      break;
    } else {
      if (level > 0) {
        // superpage
        word_t pg_mask = ((1ull << VPNiSHFT(level)) - 1);
        if ((pg_base & pg_mask) != 0) {
          // misaligned superpage
          break;
        } else if (pte.n) {
          // superpage but napot
          break;
        }
        pg_base = (pg_base & ~pg_mask) | (gpaddr & pg_mask & ~PGMASK);
      } else if (pte.n) {
        if ((pte.ppn & SVNAPOTMASK) != 0b1000) {
          break;
        }
        word_t pg_mask = ((1ull << SVNAPOTSHFT) - 1);
        pg_base = (pg_base & ~pg_mask) | (gpaddr & pg_mask & ~PGMASK);
      }
      cpu.pbmt = pte.pbmt;
      return pg_base | (gpaddr & PAGE_MASK);
    }
  }
  raise_guest_excep(gpaddr, vaddr, trap_type, is_support_vs);
  return gpaddr;
}
#endif // CONFIG_RVH


#ifndef CONFIG_MULTICORE_DIFF
static word_t pte_read(paddr_t addr, int type, int mode, vaddr_t vaddr) {
#ifdef CONFIG_SHARE
  extern bool is_in_mmio(paddr_t addr);
  if (unlikely(is_in_mmio(addr))) {
    int cause = type == MEM_TYPE_IFETCH ? EX_IAF :
                type == MEM_TYPE_WRITE  ? EX_SAF : EX_LAF;
    cpu.trapInfo.tval = vaddr;
    longjmp_exception(cause);
  }
#endif
  int paddr_read_type = type == MEM_TYPE_IFETCH ? MEM_TYPE_IFETCH_READ :
                        type == MEM_TYPE_WRITE  ? MEM_TYPE_WRITE_READ  :
                                                  MEM_TYPE_READ;
  return paddr_read(addr, PTE_SIZE, paddr_read_type, paddr_read_type, mode, vaddr);
}
#endif // CONFIG_MULTICORE_DIFF

static paddr_t ptw(vaddr_t vaddr, int type) {
  Logtr("Page walking for 0x%lx", vaddr);
  word_t pg_base = PGBASE(satp->ppn);
  int max_level;
  max_level = satp->mode == SATP_MODE_Sv39 ? 3 : 4;
  __attribute__ ((unused)) bool is_write = false;
#ifdef CONFIG_RVH
  int virt = cpu.v;
  int mode = cpu.mode;
  if(type != MEM_TYPE_IFETCH){
    if(mstatus->mprv) {
      mode = mstatus->mpp;
      virt = mstatus->mpv && mode != MODE_M;
    }
    if(hld_st){
      virt = 1;
      mode = hstatus->spvp; // spvp = 0: VU; spvp = 1: VS
    }
  }
  if(virt){
    if(vsatp->mode == SATP_MODE_BARE) return gpa_stage(vaddr, vaddr, type, type, hlvx, false) & ~PAGE_MASK;
    pg_base = PGBASE(vsatp->ppn);
    max_level = vsatp->mode == SATP_MODE_Sv39 ? 3 : 4;
  }
#endif
  bool pbmte = menvcfg->pbmte;
#ifdef CONFIG_RVH
  if(virt){
    // henvcfg.pbmte is read_only 0 when menvcfg.pbmte = 0
    pbmte = henvcfg->pbmte & menvcfg->pbmte;
  }
#endif
  word_t p_pte; // pte pointer
  PTE pte;
  int level;
  if (max_level == 4) {
    int64_t vaddr48 = vaddr << (64 - 48);
    vaddr48 >>= (64 - 48);
    if ((uint64_t)vaddr48 != vaddr) goto bad;
  } else if (max_level == 3) {
    int64_t vaddr39 = vaddr << (64 - 39);
    vaddr39 >>= (64 - 39);
    if ((uint64_t)vaddr39 != vaddr) goto bad;
  }
  for (level = max_level - 1; level >= 0;) {
    p_pte = pg_base + VPNi(vaddr, level) * PTE_SIZE;
#ifdef CONFIG_MULTICORE_DIFF
#ifdef CONFIG_RVH
    if(virt){
      p_pte = gpa_stage(p_pte, vaddr, MEM_TYPE_READ, type, false, true);
    }
#endif //CONFIG_RVH
    pte.val = golden_pmem_read(p_pte, PTE_SIZE);
#else
#ifdef CONFIG_RVH
    if(virt){
      p_pte = gpa_stage(p_pte, vaddr, MEM_TYPE_READ, type, false, true);
    }
#endif //CONFIG_RVH
    pte.val	= pte_read(p_pte, type, MODE_S, vaddr);
#endif
#ifdef CONFIG_SHARE
    if (unlikely(dynamic_config.debug_difftest)) {
      fprintf(stderr, "[NEMU] ptw: level %d, vaddr 0x%lx, pg_base 0x%lx, p_pte 0x%lx, pte.val 0x%lx\n",
        level, vaddr, pg_base, p_pte, pte.val);
    }
#endif
    pg_base = PGBASE((uint64_t)pte.ppn);
    if (!pte.v || (!pte.r && pte.w) || pte.pad) {
      goto bad;
    } else if ((ISNDEF(CONFIG_RV_SVPBMT) || !pbmte) && pte.pbmt) {
      goto bad;
    } else if (pte.pbmt == 3) {
      goto bad;
    } else if (ISNDEF(CONFIG_RV_SVNAPOT) && pte.n) {
      goto bad;
    }
    if (pte.r || pte.x) { // is leaf
      break;
    } else { // not leaf
      if (pte.a || pte.d || pte.u || pte.pbmt || pte.n) {
        goto bad;
      }
      level --;
      if (level < 0) { goto bad; }
    }
  }
#ifdef CONFIG_RVH
  if (!check_permission(&pte, true, vaddr, type, virt, mode)) return MEM_RET_FAIL;
#else
  if (!check_permission(&pte, true, vaddr, type)) return MEM_RET_FAIL;
#endif
  #ifdef CONFIG_RV_MBMC
  pt_level = level;
  #endif
  if (level > 0) {
    // superpage
    word_t pg_mask = ((1ull << VPNiSHFT(level)) - 1);
    if ((pg_base & pg_mask) != 0) {
      // missaligned superpage
      goto bad;
    } else if (pte.n) {
      // superpage but napot
      goto bad;
    }
    pg_base = (pg_base & ~pg_mask) | (vaddr & pg_mask & ~PGMASK);
  } else if (pte.n) {
    if ((pte.ppn & SVNAPOTMASK) != 0b1000) {
      goto bad;
    }
    word_t pg_mask = ((1ull << SVNAPOTSHFT) - 1);
    pg_base = (pg_base & ~pg_mask) | (vaddr & pg_mask & ~PGMASK);
  }
  #ifdef CONFIG_RVH
  if(virt){
    pg_base = gpa_stage(pg_base | (vaddr & PAGE_MASK), vaddr, type, type, hlvx, false) & ~PAGE_MASK;
    if(pg_base == MEM_RET_FAIL) return MEM_RET_FAIL;
  }
  #endif //CONFIG_RVH
#ifndef CONFIG_SHARE
  // update a/d by hardware
  is_write = (type == MEM_TYPE_WRITE);
  if (!pte.a || (!pte.d && is_write)) {
    cpu.trapInfo.tval = vaddr;
    switch (type)
    {
    int ex;
    case MEM_TYPE_IFETCH:
      longjmp_exception(EX_IPF);
      break;
    case MEM_TYPE_READ:
      ex = cpu.amo ? EX_SPF : EX_LPF;
      longjmp_exception(ex);
      break;
    case MEM_TYPE_WRITE:
      longjmp_exception(EX_SPF);
      break;
    default:
      break;
    }
  }
#endif // CONFIG_SHARE

  cpu.pbmt = pte.pbmt;
  return pg_base | MEM_RET_OK;

bad:
  Logtr("Memory translation bad");
#ifdef CONFIG_RVH
  check_permission(&pte, false, vaddr, type, virt, mode);
#else
  check_permission(&pte, false, vaddr, type);
#endif
  return MEM_RET_FAIL;
}

int ifetch_mmu_state = MMU_DIRECT;
int data_mmu_state = MMU_DIRECT;
#ifdef CONFIG_RVH
static int hyperinst_mmu_state = MMU_DIRECT; // for virtual-machine load/store instructions, HLV, HLVX, and HSV
static inline int update_hyperinst_mmu_state_internal() {
#ifdef CONFIG_RV_SV48
    assert(vsatp->mode == SATP_MODE_BARE || vsatp->mode == SATP_MODE_Sv39 || vsatp->mode == SATP_MODE_Sv48);
    assert(hgatp->mode == HGATP_MODE_BARE || hgatp->mode == HGATP_MODE_Sv39x4 || hgatp->mode == HGATP_MODE_Sv48x4);
    if (vsatp->mode == SATP_MODE_Sv39 || vsatp->mode == SATP_MODE_Sv48 || hgatp->mode == HGATP_MODE_Sv39x4 || hgatp->mode == HGATP_MODE_Sv48x4) return MMU_TRANSLATE;
#else
    assert(vsatp->mode == SATP_MODE_BARE || vsatp->mode == SATP_MODE_Sv39);
    assert(hgatp->mode == HGATP_MODE_BARE || hgatp->mode == HGATP_MODE_Sv39x4);
    if (vsatp->mode == SATP_MODE_Sv39 || hgatp->mode == HGATP_MODE_Sv39x4) return MMU_TRANSLATE;
#endif // CONFIG_RV_SV48
  return MMU_DIRECT;
}

int get_hyperinst_mmu_state() {
  return (hyperinst_mmu_state == MMU_DIRECT ? MMU_DIRECT : MMU_TRANSLATE);
}

#endif

int get_data_mmu_state() {
  return (data_mmu_state == MMU_DIRECT ? MMU_DIRECT : MMU_TRANSLATE);
}

static inline int update_mmu_state_internal(bool ifetch) {
  uint32_t mode = (mstatus->mprv && (!ifetch) && MUXDEF(CONFIG_RV_SMRNMI, mnstatus->nmie, true)
    ? mstatus->mpp : cpu.mode);
#ifdef CONFIG_RVH
  bool virt = mstatus->mprv && (!ifetch) ? mstatus->mpv && mode != MODE_M : cpu.v;
  if (mode < MODE_M) {
  #ifdef CONFIG_RV_SV48
    if (virt ? vsatp->mode == SATP_MODE_Sv39 || vsatp->mode == SATP_MODE_Sv48 || hgatp->mode == HGATP_MODE_Sv39x4 || hgatp->mode == HGATP_MODE_Sv48x4
              : satp->mode == SATP_MODE_Sv39 || satp->mode == SATP_MODE_Sv48)
      return MMU_TRANSLATE;
  #else
    if (virt ? vsatp->mode == SATP_MODE_Sv39 || hgatp->mode == HGATP_MODE_Sv39x4
              : satp->mode == SATP_MODE_Sv39)
      return MMU_TRANSLATE;
  #endif // CONFIG_RV_SV48
#else
  if (mode < MODE_M) {
  #ifdef CONFIG_RV_SV48
    if (satp->mode == SATP_MODE_Sv39 || satp->mode == SATP_MODE_Sv48) return MMU_TRANSLATE;
  #else
    if (satp->mode == SATP_MODE_Sv39) return MMU_TRANSLATE;
  #endif // CONFIG_RV_SV48
#endif // CONFIG_RVH
  }
  return MMU_DIRECT;
}

int update_mmu_state() {
  ifetch_mmu_state = update_mmu_state_internal(true);
  int data_mmu_state_old = data_mmu_state;
  data_mmu_state = update_mmu_state_internal(false);
#ifdef CONFIG_RVH
  hyperinst_mmu_state = update_hyperinst_mmu_state_internal();
#endif
  return (data_mmu_state ^ data_mmu_state_old) ? true : false;
}

void isa_misalign_data_addr_check(vaddr_t vaddr, int len, int type);

int isa_mmu_check(vaddr_t vaddr, int len, int type) {
  Logtr("MMU checking addr %lx", vaddr);
  bool is_ifetch = type == MEM_TYPE_IFETCH;

  if (!is_ifetch) {
    isa_misalign_data_addr_check(vaddr, len, type);
  }

  // riscv-privileged 4.4.1: Addressing and Memory Protection:
  // Instruction fetch addresses and load and store effective addresses,
  // which are 64 bits, must have bits 63–39 all equal to bit 38, or else a page-fault exception will occur.
#ifdef CONFIG_RVH
  bool enable_39 = satp->mode == SATP_MODE_Sv39 || ((cpu.v || hld_st) && (vsatp->mode == SATP_MODE_Sv39 || hgatp->mode == HGATP_MODE_Sv39x4));
  bool enable_48 = satp->mode == SATP_MODE_Sv48 || ((cpu.v || hld_st) && (vsatp->mode == SATP_MODE_Sv48 || hgatp->mode == HGATP_MODE_Sv48x4));
  bool vm_enable = (mstatus->mprv && (!is_ifetch) ? mstatus->mpp : cpu.mode) < MODE_M && (enable_39 || enable_48);
  bool hyperinst_vm_enable = hld_st && (vsatp->mode == SATP_MODE_Sv39 || vsatp->mode == SATP_MODE_Sv48 || hgatp->mode == HGATP_MODE_Sv39x4 || hgatp->mode == HGATP_MODE_Sv48x4);
#else
  bool enable_39 = satp->mode == SATP_MODE_Sv39;
  bool enable_48 = satp->mode == SATP_MODE_Sv48;
  bool vm_enable = (mstatus->mprv && (!is_ifetch) ? mstatus->mpp : cpu.mode) < MODE_M && (enable_39 || enable_48);
#endif

  bool va_msbs_ok = true;
  if (likely(vm_enable || MUXDEF(CONFIG_RVH, hyperinst_vm_enable, false))) {
    if (enable_48) {
      word_t va_mask = ((((word_t)1) << (63 - 47 + 1)) - 1);
      word_t va_msbs = vaddr >> 47;
      va_msbs_ok = (va_msbs == va_mask) || va_msbs == 0;
    } else if (enable_39) {
      word_t va_mask = ((((word_t)1) << (63 - 38 + 1)) - 1);
      word_t va_msbs = vaddr >> 38;
      va_msbs_ok = (va_msbs == va_mask) || va_msbs == 0;
    } else {
      Assert(0, "Invalid satp mode %d", satp->mode);
    }
  }

#ifdef CONFIG_RVH
  bool gpf = false;
  if (unlikely((cpu.v || hld_st) && vsatp->mode == SATP_MODE_BARE)) { // don't need bits 63–39 are equal to bit 38
    if (enable_48) {
      word_t maxgpa = ((((word_t)1) << 50) - 1);
      if((vaddr & ~maxgpa) == 0){
        va_msbs_ok = 1;
      }else{
        gpf = true;
      }
    } else if (enable_39) {
      word_t maxgpa = ((((word_t)1) << 41) - 1);
      if((vaddr & ~maxgpa) == 0){
        va_msbs_ok = 1;
      }else{
        gpf = true;
      }
    }
  }
#endif
  if (unlikely(!va_msbs_ok)) {
    if(is_ifetch){
      cpu.trapInfo.tval = vaddr;
#ifdef CONFIG_RVH
      if (gpf) {
        cpu.trapInfo.tval2 = vaddr >> 2;
        longjmp_exception(EX_IGPF);
      } else {
        longjmp_exception(EX_IPF);
      }
#else
      longjmp_exception(EX_IPF);
#endif
    } else if(type == MEM_TYPE_READ){
      cpu.trapInfo.tval = vaddr;
#ifdef CONFIG_RVH
      int ex;
      if(gpf){
        ex = cpu.amo ? EX_SGPF : EX_LGPF;
        cpu.trapInfo.tval2 = vaddr >> 2;
      } else {
        ex = cpu.amo ? EX_SPF : EX_LPF;
      }
      longjmp_exception(ex);
#else
      int ex = cpu.amo ? EX_SPF : EX_LPF;
      longjmp_exception(ex);
#endif
    } else {
      cpu.trapInfo.tval = vaddr;
#ifdef CONFIG_RVH
      if (gpf) {
        cpu.trapInfo.tval2 = vaddr >> 2;
        longjmp_exception(EX_SGPF);
      } else {
        longjmp_exception(EX_SPF);
      }
#else
      longjmp_exception(EX_SPF);
#endif
    }
    return MEM_RET_FAIL;
  }

  if (is_ifetch) return ifetch_mmu_state ? MMU_TRANSLATE : MMU_DIRECT;
#ifdef CONFIG_RVH
  if (hld_st)
    return hyperinst_mmu_state  ? MMU_TRANSLATE : MMU_DIRECT;
#endif
  return data_mmu_state ? MMU_TRANSLATE : MMU_DIRECT;
}

void isa_misalign_data_addr_check(vaddr_t vaddr, int len, int type) {
  if (unlikely((vaddr & (len - 1)) != 0)) {
    Logm("addr misaligned happened: vaddr:%lx len:%d type:%d pc:%lx", vaddr, len, type, cpu.pc);
    if (ISDEF(CONFIG_AC_SOFT)) {
      int ex = cpu.amo || type == MEM_TYPE_WRITE ? EX_SAM : EX_LAM;
      cpu.trapInfo.tval = vaddr;
      longjmp_exception(ex);
    }
  }
}

// Vector access currently does not support hardware misalignment.
void isa_vec_misalign_data_addr_check(vaddr_t vaddr, int len, int type) {
  if (unlikely((vaddr & (len - 1)) != 0)) {
    Logm("addr misaligned happened: vaddr:%lx len:%d type:%d pc:%lx", vaddr, len, type, cpu.pc);
    if (ISDEF(CONFIG_VECTOR_AC_SOFT)) {
      int ex = cpu.amo || type == MEM_TYPE_WRITE ? EX_SAM : EX_LAM;
      cpu.trapInfo.tval = vaddr;
      longjmp_exception(ex);
    }
  }
}

// AMO access currently does not support hardware misalignment.
void isa_amo_misalign_data_addr_check(vaddr_t vaddr, int len, int type) {
  if (unlikely((vaddr & (len - 1)) != 0)) {
    Logm("addr misaligned happened: vaddr:%lx len:%d type:%d pc:%lx", vaddr, len, type, cpu.pc);
    if (ISDEF(CONFIG_AMO_AC_SOFT)) {
      int ex = cpu.amo || type == MEM_TYPE_WRITE ? EX_SAM : EX_LAM;
      cpu.trapInfo.tval = vaddr;
      longjmp_exception(ex);
    }
  }
}

paddr_t isa_mmu_translate(vaddr_t vaddr, int len, int type) {
  paddr_t ptw_result = ptw(vaddr, type);
#ifdef FORCE_RAISE_PF
#ifdef CONFIG_RVH
  if(ptw_result != MEM_RET_FAIL && (force_raise_pf(vaddr, type) != MEM_RET_OK || force_raise_gpf(vaddr, type) != MEM_RET_OK))
    return MEM_RET_FAIL;
#else
  if(ptw_result != MEM_RET_FAIL && force_raise_pf(vaddr, type) != MEM_RET_OK)
    return MEM_RET_FAIL;
#endif // CONFIG_RVH
#endif // FORCE_RAISE_PF
  return ptw_result;
}

int force_raise_pf_record(vaddr_t vaddr, int type) {
  static vaddr_t last_addr[3] = {0x0};
  static int force_count[3] = {0};
  if (vaddr != last_addr[type]) {
    last_addr[type] = vaddr;
    force_count[type] = 0;
  }
  force_count[type]++;
  return force_count[type] == 5;
}

int force_raise_pf(vaddr_t vaddr, int type){
  bool ifetch = (type == MEM_TYPE_IFETCH);

  if(cpu.guided_exec && cpu.execution_guide.force_raise_exception){
    if(ifetch && cpu.execution_guide.exception_num == EX_IPF){
      if (force_raise_pf_record(vaddr, type)) {
        return MEM_RET_OK;
      }
#ifdef CONFIG_RVH
      if (intr_deleg_VS(EX_IPF)) {
        cpu.trapInfo.tval = cpu.execution_guide.vstval;
        if(
          vaddr != cpu.execution_guide.vstval &&
          // cross page ipf caused mismatch is legal
          !((vaddr & 0xfff) == 0xffe && (cpu.execution_guide.vstval & 0xfff) == 0x000)
        ){
          printf("[WARNING] nemu vstval %lx does not match core vstval %lx\n",
            vaddr,
            cpu.execution_guide.vstval
          );
        }
      } else if (intr_deleg_S(EX_IPF)) {
#else
      if(intr_deleg_S(EX_IPF)) {
#endif // CONFIG_RVH
        cpu.trapInfo.tval = cpu.execution_guide.stval;
        if(
          vaddr != cpu.execution_guide.stval &&
          // cross page ipf caused mismatch is legal
          !((vaddr & 0xfff) == 0xffe && (cpu.execution_guide.stval & 0xfff) == 0x000)
        ){
          printf("[WARNING] nemu stval %lx does not match core stval %lx\n",
            vaddr,
            cpu.execution_guide.stval
          );
        }
      } else {
        cpu.trapInfo.tval = cpu.execution_guide.mtval;
        if(
          vaddr != cpu.execution_guide.mtval &&
          // cross page ipf caused mismatch is legal
          !((vaddr & 0xfff) == 0xffe && (cpu.execution_guide.mtval & 0xfff) == 0x000)
        ){
          printf("[WARNING] nemu mtval %lx does not match core mtval %lx\n",
            vaddr,
            cpu.execution_guide.mtval
          );
        }
      }
      printf("[NEMU]: force raise IPF\n");
      longjmp_exception(EX_IPF);
      return MEM_RET_FAIL;
    } else if(!ifetch && type == MEM_TYPE_READ && cpu.execution_guide.exception_num == EX_LPF){
      if (force_raise_pf_record(vaddr, type)) {
        return MEM_RET_OK;
      }

#ifdef CONFIG_GUIDED_TVAL
      if (vaddr != SELECT_DUT_INTR_TVAL_REG(EX_LPF)) return MEM_RET_OK;
#endif
      printf("[NEMU]: force raise LPF\n");

      cpu.trapInfo.tval = vaddr;
      longjmp_exception(EX_LPF);
      return MEM_RET_FAIL;
    } else if(type == MEM_TYPE_WRITE && cpu.execution_guide.exception_num == EX_SPF){
      if (force_raise_pf_record(vaddr, type)) {
        return MEM_RET_OK;
      }

#ifdef CONFIG_GUIDED_TVAL
      if (vaddr != SELECT_DUT_INTR_TVAL_REG(EX_SPF)) return MEM_RET_OK;
#endif
      printf("[NEMU]: force raise SPF\n");

      cpu.trapInfo.tval = vaddr;
      longjmp_exception(EX_SPF);
      return MEM_RET_FAIL;
    }
  }
  return MEM_RET_OK;
}

#ifdef CONFIG_RVH
int force_raise_gpf_record(vaddr_t vaddr, int type) {
  static vaddr_t g_last_addr[3] = {0x0};
  static int g_force_count[3] = {0};
  if (vaddr != g_last_addr[type]) {
    g_last_addr[type] = vaddr;
    g_force_count[type] = 0;
  }
  g_force_count[type]++;
  return g_force_count[type] == 5;
}

int force_raise_gpf(vaddr_t vaddr, int type){
  bool ifetch = (type == MEM_TYPE_IFETCH);

  if(cpu.guided_exec && cpu.execution_guide.force_raise_exception){
    if(ifetch && cpu.execution_guide.exception_num == EX_IGPF){
      if (force_raise_gpf_record(vaddr, type)) {
        return MEM_RET_OK;
      }
      if (intr_deleg_S(EX_IGPF)) {
        cpu.trapInfo.tval = cpu.execution_guide.stval;
        cpu.trapInfo.tval2 = cpu.execution_guide.htval;
        if(
          vaddr != cpu.execution_guide.stval &&
          // cross page ipf caused mismatch is legal
          !((vaddr & 0xfff) == 0xffe && (cpu.execution_guide.stval & 0xfff) == 0x000)
        ){
          printf("[WARNING] nemu stval %lx does not match core stval %lx\n",
            vaddr,
            cpu.execution_guide.stval
          );
        }
      } else {
        cpu.trapInfo.tval = cpu.execution_guide.mtval;
        cpu.trapInfo.tval2 = cpu.execution_guide.mtval2;
        if(
          vaddr != cpu.execution_guide.mtval &&
          // cross page ipf caused mismatch is legal
          !((vaddr & 0xfff) == 0xffe && (cpu.execution_guide.mtval & 0xfff) == 0x000)
        ){
          printf("[WARNING] nemu mtval %lx does not match core mtval %lx\n",
            vaddr,
            cpu.execution_guide.mtval
          );
        }
      }
      printf("[NEMU]: force raise IGPF\n");
      longjmp_exception(EX_IGPF);
      return MEM_RET_FAIL;
    } else if(!ifetch && type == MEM_TYPE_READ && cpu.execution_guide.exception_num == EX_LGPF){
      if (force_raise_gpf_record(vaddr, type)) {
        return MEM_RET_OK;
      }

#ifdef CONFIG_GUIDED_TVAL
      if (vaddr != SELECT_DUT_INTR_TVAL_REG(EX_LGPF)) return MEM_RET_OK;
#endif
      printf("[NEMU]: force raise LGPF\n");

      cpu.trapInfo.tval = vaddr;
      cpu.trapInfo.tval2 = intr_deleg_S(EX_LGPF) ? cpu.execution_guide.htval: cpu.execution_guide.mtval2;
      longjmp_exception(EX_LGPF);
      return MEM_RET_FAIL;
    } else if(type == MEM_TYPE_WRITE && cpu.execution_guide.exception_num == EX_SGPF){
      if (force_raise_gpf_record(vaddr, type)) {
        return MEM_RET_OK;
      }

#ifdef CONFIG_GUIDED_TVAL
      if (vaddr != SELECT_DUT_INTR_TVAL_REG(EX_SGPF)) return MEM_RET_OK;
#endif
      printf("[NEMU]: force raise SGPF\n");

      cpu.trapInfo.tval = vaddr;
      cpu.trapInfo.tval2 = intr_deleg_S(EX_SGPF) ? cpu.execution_guide.htval: cpu.execution_guide.mtval2;
      longjmp_exception(EX_SGPF);
      return MEM_RET_FAIL;
    }
  }
  return MEM_RET_OK;
}

#endif // CONFIG_RVH

#ifdef CONFIG_PMPTABLE_EXTENSION
static bool napot_decode(paddr_t addr, word_t pmpaddr) {
  word_t pmpaddr_start, pmpaddr_end;
  /* NAPOT decode method, learn form qemu */
  pmpaddr_start = (pmpaddr & (pmpaddr + 1)) << PMP_SHIFT;
  pmpaddr_end = (pmpaddr | (pmpaddr + 1)) << PMP_SHIFT;
  return ((pmpaddr_start <= addr && addr < pmpaddr_end) ? true : false);
}

static uint8_t pmp_address_match(paddr_t base, paddr_t addr, int len, word_t pmpaddr, uint8_t addr_mode) {
  /* start address and end address */
  paddr_t addr_s, addr_e;
  addr_s = addr;
  addr_e = addr + len;
  /* matched flag of start address and end address */
  uint8_t s_flag = 0;
  uint8_t e_flag = 0;

  /* TOR: use last pmpaddr(base) as floor, and current pmpaddr as roof*/
  if (addr_mode == PMP_TOR) {
    pmpaddr = pmpaddr << PMP_SHIFT;
    s_flag = (base <= addr_s && addr_s < pmpaddr ) ? 1 : 0;
    e_flag = (base <= addr_e && addr_e < pmpaddr) ? 1 : 0;
  }
  /* NA4: pmpaddr ~ (pmpaddr + 4) */
  else if (addr_mode == PMP_NA4) {
    pmpaddr = pmpaddr << PMP_SHIFT;
    s_flag = (pmpaddr <= addr_s && addr_s < (pmpaddr + (1 << PMP_SHIFT))) ? 1 : 0;
    e_flag = (pmpaddr <= addr_e && addr_e < (pmpaddr + (1 << PMP_SHIFT))) ? 1 : 0;
  }
  /* NAPOT: decode the NAPOT format pmpaddr */
  else if (addr_mode == PMP_NAPOT) {
    s_flag = napot_decode(addr_s, pmpaddr) ? 1 : 0;
    e_flag = napot_decode(addr_e, pmpaddr) ? 1 : 0;
  }
  return s_flag + e_flag;
}

bool pmpcfg_check_permission(uint8_t pmpcfg,int type,int out_mode) {
  if (out_mode == MODE_M) {
    return true;
  }
  else {
    if (type == MEM_TYPE_READ || type == MEM_TYPE_IFETCH_READ
        || type == MEM_TYPE_WRITE_READ)
      return pmpcfg & PMP_R;
    else if (type == MEM_TYPE_WRITE)
      return pmpcfg & PMP_W;
    else if (type == MEM_TYPE_IFETCH)
      return pmpcfg & PMP_X;
    else {
      Log("Wrong memory access type: %d!", type);
      return false;
    }
  }
}

bool pmptable_check_permission(word_t offset, word_t root_table_base, int type, int out_mode) {
  if (out_mode == MODE_M) {
    return true;
  }
  else {
    uint64_t off1 = (offset >> 25) & 0x1ff;       /* root table offset */
    uint64_t off0 = (offset >> 16) & 0x1ff;       /* leaf table offset */
    uint8_t page_index = (offset >> 12) & 0xf;    /* page index */
    uint8_t perm = 0;                             /* permission, default no permission */

    uint64_t root_pte_addr = root_table_base + (off1 << 3);
    /*
     * Get root pte:
     * Use host_read instead of paddr_read, avoid nested call of isa_pmp_check_permission
     */
    uint64_t root_pte = host_read(guest_to_host(root_pte_addr), 8);

    /*
     * root_pte case(last 4 bits are 0001):
     * valid(last bit is 1) but no permission bit is set(other bit are all 0),
     * should check the leaf table to get permission.
     */
    if ((root_pte & 0x0f) == 1) {
      bool at_high = page_index % 2;
      int idx = page_index / 2;
      uint8_t leaf_pte = host_read(guest_to_host(((root_pte >> 5) << 12) + (off0 << 3)) + idx, 1);
      if (at_high) {
        perm = leaf_pte >> 4;
      }
      else {
        perm = leaf_pte & 0xf;
      }
    }
    /*
     * root_pte case(last 4 bits are xxx1):
     * valid(last bit is 1) and some permission bits are set(other bit are not all 0),
     * directly use the root pte to get permission.
     */
    else if ((root_pte & 0x1) == 1) {
      perm = (root_pte >> 1) & 0xf;
    }
    /*
     * root_pte case(last 4 bits are xxx0):
     * invaild(last bit is 0), directly return false.
     */
    else {
      return false;
    }

#define R_BIT 0x1
#define W_BIT 0x2
#define X_BIT 0x4
    /* Check permission */
    if (type == MEM_TYPE_READ || type == MEM_TYPE_IFETCH_READ
        || type == MEM_TYPE_WRITE_READ) {
      return perm & R_BIT;
    }
    else if (type == MEM_TYPE_WRITE) {
      return perm & W_BIT;
    }
    else if (type == MEM_TYPE_IFETCH) {
      return perm & X_BIT;
    }
    else {
      Log("pmptable get wrong type of memory access!");
      return false;
    }
#undef R_BIT
#undef W_BIT
#undef X_BIT
  }
}
#endif

#ifdef CONFIG_RV_MBMC
bool isa_bmc_check_permission(paddr_t addr, int len, int type, int out_mode) {
  if (mbmc->BME == 0) {
    return true;
  }
  if (mbmc->CMODE == 1) {
    return true;
  }
  word_t bm_base = (mbmc->BMA) << 6;
  // word_t ppn = ((addr >> PGSHFT));
  word_t ppn = (addr >> (9 * pt_level + PGSHFT) << (9 * pt_level));
  bool is_bmc = (bitmap_read(bm_base + ppn / 8, MEM_TYPE_BM_READ, out_mode) >> (ppn % 8)) & 1;
  return !is_bmc;
}
#endif

bool isa_pmp_check_permission(paddr_t addr, int len, int type, int out_mode) {
  bool ifetch = (type == MEM_TYPE_IFETCH);
  __attribute__((unused)) uint32_t mode;
  mode = (out_mode == MODE_M) ? (mstatus->mprv && !ifetch ? mstatus->mpp : cpu.mode) : out_mode;
  // paddr_read/write method may not be able pass down the 'effective' mode for isa difference. do it here
#ifdef CONFIG_SHARE
  // if(dynamic_config.debug_difftest) {
  //   if (mode != out_mode) {
  //     fprintf(stderr, "[NEMU]   PMP out_mode:%d cpu.mode:%ld ifetch:%d mprv:%d mpp:%d actual mode:%d\n", out_mode, cpu.mode, ifetch, mstatus->mprv, mstatus->mpp, mode);
  //       // Log("addr:%lx len:%d type:%d out_mode:%d mode:%d", addr, len, type, out_mode, mode);
  //   }
  // }
#endif

#ifdef CONFIG_RV_PMP_CHECK
  if (CONFIG_RV_PMP_ACTIVE_NUM == 0) {
    return true;
  }

  word_t base = 0;
  for (int i = 0; i < CONFIG_RV_PMP_ACTIVE_NUM; i++) {
    word_t pmpaddr = pmpaddr_from_index(i);
    word_t tor = (pmpaddr & pmp_tor_mask()) << PMP_SHIFT;
    uint8_t cfg = pmpcfg_from_index(i);

    if (cfg & PMP_A) {
      bool is_tor = (cfg & PMP_A) == PMP_TOR;
      bool is_na4 = (cfg & PMP_A) == PMP_NA4;

      word_t mask = (pmpaddr << 1) | (!is_na4) | ~pmp_tor_mask();
      mask = ~(mask & ~(mask + 1)) << PMP_SHIFT;

      // Check each 4-byte sector of the access
      bool any_match = false;
      bool all_match = true;
      for (word_t offset = 0; offset < len; offset += 1 << PMP_SHIFT) {
        word_t cur_addr = addr + offset;
        bool napot_match = ((cur_addr ^ tor) & mask) == 0;
        bool tor_match = base <= cur_addr && cur_addr < tor;
        bool match = is_tor ? tor_match : napot_match;
        any_match |= match;
        all_match &= match;
#ifdef CONFIG_SHARE
        // if(dynamic_config.debug_difftest) {
        //   fprintf(stderr, "[NEMU]   PMP byte match %ld addr:%016lx cur_addr:%016lx tor:%016lx mask:%016lx base:%016lx match:%s\n",
        //   offset, addr, cur_addr, tor, mask, base, match ? "true" : "false");
        // }
#endif
      }
#ifdef CONFIG_SHARE
        // if(dynamic_config.debug_difftest) {
        //   fprintf(stderr, "[NEMU]   PMP %d cfg:%02x pmpaddr:%016lx isna4:%d isnapot:%d istor:%d base:%016lx addr:%016lx any_match:%d\n",
        //     i, cfg, pmpaddr, is_na4, !is_na4 && !is_tor, is_tor, base, addr, any_match);
        // }
#endif
      if (any_match) {
        // If the PMP matches only a strict subset of the access, fail it
        if (!all_match) {
#ifdef CONFIG_SHARE
          // if(dynamic_config.debug_difftest) {
          //   fprintf(stderr, "[NEMU]   PMP addr:0x%016lx len:%d type:%d mode:%d pass:false for not all match\n", addr, len, type, mode);
          // }
#endif
          return false;
        }

#ifdef CONFIG_SHARE
        // if(dynamic_config.debug_difftest) {
        //   bool pass = (mode == MODE_M && !(cfg & PMP_L)) ||
        //       ((type == MEM_TYPE_READ || type == MEM_TYPE_IFETCH_READ ||
        //         type == MEM_TYPE_WRITE_READ) && (cfg & PMP_R)) ||
        //       (type == MEM_TYPE_WRITE && (cfg & PMP_W)) ||
        //       (type == MEM_TYPE_IFETCH && (cfg & PMP_X));
        //   fprintf(stderr, "[NEMU]   PMP %d cfg:%02x pmpaddr:%016lx addr:0x%016lx len:%d type:%d mode:%d pass:%s \n", i, cfg, pmpaddr, addr, len, type, mode,
        //       pass ? "true" : "false for permission denied");
        // }
#endif

        return
          (mode == MODE_M && !(cfg & PMP_L)) ||
          ((type == MEM_TYPE_READ || type == MEM_TYPE_IFETCH_READ ||
            type == MEM_TYPE_WRITE_READ) && (cfg & PMP_R)) ||
          (type == MEM_TYPE_WRITE && (cfg & PMP_W)) ||
          (type == MEM_TYPE_IFETCH && (cfg & PMP_X));
      }
    }

    base = tor;
  }

#ifdef CONFIG_SHARE
  // if(dynamic_config.debug_difftest) {
  //   if (mode != MODE_M) fprintf(stderr, "[NEMU]   PMP addr:0x%016lx len:%d type:%d mode:%d pass:%s\n", addr, len, type, mode,
  //   mode == MODE_M ? "true for mode m but no match" : "false for no match with less than M mode");
  // }
#endif

  return mode == MODE_M;

#endif

#ifdef CONFIG_PMPTABLE_EXTENSION
  if (CONFIG_RV_PMP_ACTIVE_NUM == 0) {
    return true;
  }
  int i = 0;
  word_t base = 0;
  for (i = 0; i < CONFIG_RV_PMP_ACTIVE_NUM; i++) {
    uint8_t pmpcfg = pmpcfg_from_index(i);
    word_t pmpaddr = pmpaddr_from_index(i);
    uint8_t addr_mode = pmpcfg & PMP_A;
    if (addr_mode) {
      int match_ret = 0;
      match_ret = pmp_address_match(base, addr, len, pmpaddr, addr_mode);
      /*
       * When match_ret == 1, means that only a part of addr is in a pmpaddr region
       * and it is illegal.
       */
      if (match_ret == 1) {
        Log("[ERROR] addr is illegal in pmpaddr match. pmpcfg[%d] = %#x", i, pmpcfg);
        return false;
      }
      /* Not matched */
      else if (match_ret == 0){
        continue;
      }
      /* Matched */
      else {
        /* Table-bit is enabled, get permission from pmptable */
        if (pmpcfg & PMP_T) {
          word_t offset = 0;
          if (addr_mode == PMP_TOR){
            offset = addr - base;
          }
          else {
            offset = addr - (pmpaddr << PMP_SHIFT);
          }
          word_t root_table_base = pmpaddr_from_index(i + 1) << 12;
          return pmptable_check_permission(offset, root_table_base, type, out_mode);
        }
        /* Table-bit is disable, get permission directly form pmpcfg reg */
        else {
          return pmpcfg_check_permission(pmpcfg, type, out_mode);
        }
      }
    }
    base = pmpaddr << PMP_SHIFT;
  }
  return true;
#endif

#ifndef CONFIG_RV_PMP_CHECK
#ifndef CONFIG_PMPTABLE_EXTENSION
  return true;
#endif
#endif
}

bool isa_pma_check_permission(paddr_t addr, int len, int type) {
#ifdef CONFIG_RV_PMA_CHECK
  if (CONFIG_RV_PMA_ACTIVE_NUM == 0) {
    return true;
  }

  word_t base = 0;
  for (int i = 0; i < CONFIG_RV_PMA_ACTIVE_NUM; i++) {
    word_t pmaaddr = pmaaddr_from_index(i);
    word_t tor = (pmaaddr & pma_tor_mask()) << PMA_SHIFT;
    uint8_t cfg = pmacfg_from_index(i);

    if (cfg & PMA_A) {
      bool is_tor = (cfg & PMA_A) == PMA_TOR;
      bool is_na4 = (cfg & PMA_A) == PMA_NA4;

      word_t mask = (pmaaddr << 1) | (!is_na4) | ~pma_tor_mask();
      mask = ~(mask & ~(mask + 1)) << PMA_SHIFT;

      // Check each 4-byte sector of the access
      bool any_match = false;
      bool all_match = true;
      for (word_t offset = 0; offset < len; offset += 1 << PMA_SHIFT) {
        word_t cur_addr = addr + offset;
        bool napot_match = ((cur_addr ^ tor) & mask) == 0;
        bool tor_match = base <= cur_addr && cur_addr < tor;
        bool match = is_tor ? tor_match : napot_match;
        any_match |= match;
        all_match &= match;
      }
      if (any_match) {
        if (!all_match) {
          return false;
        }
        return 
          ((type == MEM_TYPE_READ || type == MEM_TYPE_IFETCH_READ ||
            type == MEM_TYPE_WRITE_READ) && (cfg & PMA_R)) ||
          (type == MEM_TYPE_WRITE && (cfg & PMA_W)) ||
          (type == MEM_TYPE_IFETCH && (cfg & PMA_X)) ||
          ((type == MEM_TYPE_READ || type == MEM_TYPE_WRITE ||
            type == MEM_TYPE_WRITE_READ) && (cfg & PMA_T)) ||
          ((type == MEM_TYPE_READ || type == MEM_TYPE_WRITE ||
            type == MEM_TYPE_WRITE_READ) && (cfg & PMA_C)) ;
      }
    }

    base = tor;
  }
  return true;
#endif

#ifndef CONFIG_RV_PMA_CHECK
  return true;
#endif
}
