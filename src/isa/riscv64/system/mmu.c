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
    uint32_t pad :10;
  };
  uint64_t val;
} PTE;

#define PGSHFT 12
#define PGMASK ((1ull << PGSHFT) - 1)
#define PGBASE(pn) (pn << PGSHFT)

// Sv39 page walk
#define PTW_LEVEL 3
#define PTE_SIZE 8
#define VPNMASK 0x1ff
#define GPVPNMASK 0x7ff
static inline uintptr_t VPNiSHFT(int i) {
  return (PGSHFT) + 9 * i;
}
static inline uintptr_t VPNi(vaddr_t va, int i) {
  return (va >> VPNiSHFT(i)) & VPNMASK;
}
#ifdef CONFIG_RVH
static inline uintptr_t GVPNi(vaddr_t va, int i) {
  return (i == 2)?  (va >> VPNiSHFT(i)) & GPVPNMASK : (va >> VPNiSHFT(i)) & VPNMASK;
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
#ifdef CONFIG_RVH
  ok = ok && !(pte->u && ((mode == MODE_S) && (!(virt? ((hstatus->vsxl == 1)? vsstatus->_32.sum  : vsstatus->_64.sum): mstatus->sum) || ifetch)));
  Logtr("ok: %i, mode == U: %i, pte->u: %i, ppn: %lx, virt: %d", ok, mode == MODE_U, pte->u, (uint64_t)pte->ppn << 12, virt);
#else
  ok = ok && !(pte->u && ((mode == MODE_S) && (!mstatus->sum || ifetch)));
  Logtr("ok: %i, mode == U: %i, pte->u: %i, ppn: %lx", ok, mode == MODE_U, pte->u, (uint64_t)pte->ppn << 12);
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
      INTR_TVAL_REG(EX_IPF) = vaddr;
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
    can_load = pte->r || ((mstatus->mxr || (((hstatus->vsxl == 1)? vsstatus->_32.mxr  : vsstatus->_64.mxr) && virt)) && pte->x);
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
      INTR_TVAL_REG(ex) = vaddr;
      cpu.amo = false;
      Logtr("Memory read translation exception!");
      longjmp_exception(ex);
      return false;
    }
  } else {
#ifdef CONFIG_SHARE
    bool update_ad = !pte->a || !pte->d;
   if (update_ad && ok && pte->w) Logtr("raise exception to update ad for store");
#else
    bool update_ad = false;
#endif
    Logtr("Translate for memory writing");
    if (!(ok && pte->w && !pte->pad) || update_ad) {
      INTR_TVAL_REG(EX_SPF) = vaddr;
      cpu.amo = false;
      longjmp_exception(EX_SPF);
      return false;
    }
  }
  return true;
}
#ifdef CONFIG_RVH
bool has_two_stage_translation(){
  return hld_st || (mstatus->mprv && mstatus->mpv) || cpu.v;
}

void raise_guest_excep(paddr_t gpaddr, vaddr_t vaddr, int type){
  // printf("gpaddr: %lx, vaddr: %lx\n", gpaddr, vaddr);
  if (type == MEM_TYPE_IFETCH){
    if(intr_deleg_S(EX_IGPF)){
      stval->val = vaddr;
      htval->val = gpaddr >> 2;
    }else{
      mtval->val = vaddr;
      mtval2->val = gpaddr >> 2;
    }
    longjmp_exception(EX_IGPF);
  }else if (type == MEM_TYPE_READ){
    int ex = cpu.amo ? EX_SGPF : EX_LGPF;
    if(intr_deleg_S(ex)){
      stval->val = vaddr;
      htval->val = gpaddr >> 2;
    }else{
      mtval->val = vaddr;
      mtval2->val = gpaddr >> 2;
    }
    longjmp_exception(ex);
  }else{
    if(intr_deleg_S(EX_SGPF)){
      stval->val = vaddr;
      htval->val = gpaddr >> 2;
    }else{
      mtval->val = vaddr;
      mtval2->val = gpaddr >> 2;
    }
    longjmp_exception(EX_SGPF);
  }
}

paddr_t gpa_stage(paddr_t gpaddr, vaddr_t vaddr, int type){
  Logtr("gpa_stage gpaddr: 0x%lx, vaddr: 0x%lx, type: %d", gpaddr, vaddr, type);
  if(hgatp->mode == 8){
    if((gpaddr & ~(((int64_t)1 << 41) - 1)) != 0){
      raise_guest_excep(gpaddr, vaddr, type);
    }
    word_t pg_base = PGBASE(hgatp->ppn);
    int level;
    word_t p_pte;
    PTE pte;
    for (level = PTW_LEVEL - 1; level >=0;){
      p_pte = pg_base + GVPNi(gpaddr, level) * PTE_SIZE;
      pte.val	= paddr_read(p_pte, PTE_SIZE,
      type == MEM_TYPE_IFETCH ? MEM_TYPE_IFETCH_READ :
      type == MEM_TYPE_WRITE ? MEM_TYPE_WRITE_READ : MEM_TYPE_READ, MODE_S, vaddr);
      #ifdef CONFIG_SHARE
          if (unlikely(dynamic_config.debug_difftest)) {
            fprintf(stderr, "[NEMU] ptw g stage: level %d, vaddr 0x%lx, gpaddr 0x%lx, pg_base 0x%lx, p_pte 0x%lx, pte.val 0x%lx\n",
              level, vaddr, gpaddr, pg_base, p_pte, pte.val);
          }
      #endif
      pg_base = PGBASE(pte.ppn);
      Logtr("g p_pte: %lx pg base:0x%lx, v:%d, r:%d, w: %d, x: %d", p_pte, pg_base, pte.v, pte.r, pte.w, pte.x);
      if(pte.v && !pte.r && !pte.w && !pte.x){
        level --;
        if (level < 0) { break; }
      }else if (!pte.v || (!pte.r && pte.w))
        break;
      else if(!pte.u)
        break;
      else if(type == MEM_TYPE_IFETCH || hlvx ? !pte.x:
              type == MEM_TYPE_READ           ? !pte.r && !(mstatus->mxr && pte.x):
                                                !(pte.r && pte.w))
        break;
      else{
         if (level > 0) {
          // superpage
          word_t pg_mask = ((1ull << VPNiSHFT(level)) - 1);
          if ((pg_base & pg_mask) != 0) {
            // missaligned superpage
            return MEM_RET_FAIL;
          }
          pg_base = (pg_base & ~pg_mask) | (gpaddr & pg_mask & ~PGMASK);
        }
        return pg_base | (gpaddr & PAGE_MASK);
      }
    }
    raise_guest_excep(gpaddr, vaddr, type);
  }
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
    INTR_TVAL_REG(cause) = vaddr;
    longjmp_exception(cause);
  }
#endif
  int paddr_read_type = type == MEM_TYPE_IFETCH ? MEM_TYPE_IFETCH_READ :
                        type == MEM_TYPE_WRITE  ? MEM_TYPE_WRITE_READ  :
                                                  MEM_TYPE_READ;
  return paddr_read(addr, PTE_SIZE, paddr_read_type, mode, vaddr);
}
#endif // CONFIG_MULTICORE_DIFF

static paddr_t ptw(vaddr_t vaddr, int type) {
  Logtr("Page walking for 0x%lx\n", vaddr);
  word_t pg_base = PGBASE(satp->ppn);
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
    if(vsatp_mode == 0) return gpa_stage(vaddr, vaddr, type) & ~PAGE_MASK;
    pg_base = PGBASE(vsatp_ppn);
  }
#endif
  word_t p_pte; // pte pointer
  PTE pte;
  int level;
  int64_t vaddr39 = vaddr << (64 - 39);
  vaddr39 >>= (64 - 39);
  if ((uint64_t)vaddr39 != vaddr) goto bad;
  for (level = PTW_LEVEL - 1; level >= 0;) {
    p_pte = pg_base + VPNi(vaddr, level) * PTE_SIZE;
#ifdef CONFIG_MULTICORE_DIFF
    pte.val = golden_pmem_read(p_pte, PTE_SIZE, 0, 0, 0);
#else
  #ifdef CONFIG_RVH
    if(virt){
      p_pte = gpa_stage(p_pte, vaddr, type);
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
    if (!pte.v || (!pte.r && pte.w)) goto bad;
    if (pte.r || pte.x || pte.pad) { break; }
    else {
      level --;
      if (level < 0) { goto bad; }
    }
  }
#ifdef CONFIG_RVH
  if (!check_permission(&pte, true, vaddr, type, virt, mode)) return MEM_RET_FAIL;
#else
  if (!check_permission(&pte, true, vaddr, type)) return MEM_RET_FAIL;
#endif
  if (level > 0) {
    // superpage
    word_t pg_mask = ((1ull << VPNiSHFT(level)) - 1);
    if ((pg_base & pg_mask) != 0) {
      // missaligned superpage
      goto bad;
    }
    pg_base = (pg_base & ~pg_mask) | (vaddr & pg_mask & ~PGMASK);
  }
  #ifdef CONFIG_RVH
  if(virt){
    pg_base = gpa_stage(pg_base | (vaddr & PAGE_MASK), vaddr, type) & ~PAGE_MASK;
    if(pg_base == MEM_RET_FAIL) return MEM_RET_FAIL;
  }
  #endif //CONFIG_RVH
#ifndef CONFIG_SHARE
  // update a/d by hardware
  bool is_write = (type == MEM_TYPE_WRITE);
  if (!pte.a || (!pte.d && is_write)) {
    // pte.a = true;
    // pte.d |= is_write;
    // paddr_write(p_pte, PTE_SIZE, pte.val, cpu.mode, vaddr);
    switch (type)
    {
    int ex;
    case MEM_TYPE_IFETCH:
#ifdef CONFIG_RVH
      if(cpu.v){
        if(intr_deleg_S(EX_IPF)){
          vstval->val = vaddr;
        }else{
          mtval->val = vaddr;
        }
        longjmp_exception(EX_IPF);
      }else{
        INTR_TVAL_REG(EX_IPF) = vaddr;
        longjmp_exception(EX_IPF);
      }
#else
      stval->val = vaddr;
      INTR_TVAL_REG(EX_IPF) = vaddr;
      longjmp_exception(EX_IPF);
#endif
      break;
    case MEM_TYPE_READ:
#ifdef CONFIG_RVH
      if(cpu.v){
        ex = cpu.amo ? EX_SPF : EX_LPF;
        if(intr_deleg_S(ex)){
          vstval->val = vaddr;
        }else{
          mtval->val = vaddr;
        }
      }else{
        ex = cpu.amo ? EX_SPF : EX_LPF;
        INTR_TVAL_REG(ex) = vaddr;
      }
      longjmp_exception(ex);
#else
      ex = cpu.amo ? EX_SPF : EX_LPF;
      INTR_TVAL_REG(ex) = vaddr;
      longjmp_exception(ex);
#endif
      break;
    case MEM_TYPE_WRITE:
#ifdef CONFIG_RVH
      if(cpu.v){
        if(intr_deleg_S(EX_SPF)){
          vstval->val = vaddr;
        }else{
          mtval->val = vaddr;
        }
        longjmp_exception(EX_SPF);
      }else{
        INTR_TVAL_REG(EX_SPF) = vaddr;
        longjmp_exception(EX_SPF);
      }
#else
      INTR_TVAL_REG(EX_SPF) = vaddr;
      longjmp_exception(EX_SPF);
#endif
      break;
    default:
      break;
    }
  }
#endif

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
static int h_mmu_state = MMU_DIRECT;
static inline int update_h_mmu_state_internal(bool ifetch) {
  uint32_t mode = (mstatus->mprv && (!ifetch) ? mstatus->mpp : cpu.mode);
  if (mode < MODE_M) {
    assert(vsatp_mode == 0 || vsatp_mode == 8);
    assert(hgatp->mode == 0 || hgatp->mode == 8);
    if (vsatp_mode == 8 || hgatp->mode == 8) return MMU_TRANSLATE;
  }
  return MMU_DIRECT;
}
int get_h_mmu_state() {
  return (h_mmu_state == MMU_DIRECT ? MMU_DIRECT : MMU_TRANSLATE);
}
#endif

int get_data_mmu_state() {
  return (data_mmu_state == MMU_DIRECT ? MMU_DIRECT : MMU_TRANSLATE);
}

static inline int update_mmu_state_internal(bool ifetch) {
  uint32_t mode = (mstatus->mprv && (!ifetch) ? mstatus->mpp : cpu.mode);
  if (mode < MODE_M) {
    assert(satp->mode == 0 || satp->mode == 8);
    if (satp->mode == 8) return MMU_TRANSLATE;
  }
  return MMU_DIRECT;
}

int update_mmu_state() {
  ifetch_mmu_state = update_mmu_state_internal(true);
  int data_mmu_state_old = data_mmu_state;
  data_mmu_state = update_mmu_state_internal(false);
#ifdef CONFIG_RVH
  h_mmu_state = update_h_mmu_state_internal(false);
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
  bool vm_enable = (mstatus->mprv && (!is_ifetch) ? mstatus->mpp : cpu.mode) < MODE_M && (satp->mode == 8 || (cpu.v && (vsatp_mode == 8 || hgatp->mode == 8)));
#else
  bool vm_enable = (mstatus->mprv && (!is_ifetch) ? mstatus->mpp : cpu.mode) < MODE_M && satp->mode == 8;
#endif
  word_t va_mask = ((((word_t)1) << (63 - 38 + 1)) - 1);
  word_t va_msbs = vaddr >> 38;
  bool va_msbs_ok = (va_msbs == va_mask) || va_msbs == 0 || !vm_enable;
#ifdef CONFIG_RVH
  bool gpf = false;
  if(cpu.v && vsatp_mode == 0){ // don't need bits 63–39 are equal to bit 38
    word_t maxgpa = ((((word_t)1) << 41) - 1);
    if((vaddr & ~maxgpa) == 0){
      va_msbs_ok = 1;
    }else{
      gpf = true;
    }
  }
#endif
  if(!va_msbs_ok){
    if(is_ifetch){
#ifdef CONFIG_RVH
      if(hld_st || gpf){
        if(intr_deleg_S(EX_IGPF)){
          stval->val = vaddr;
          htval->val = vaddr >> 2;
        }else{
          mtval->val = vaddr;
          mtval2->val = vaddr >> 2;
        }
        longjmp_exception(EX_IGPF);
      }else if(cpu.v){
        if(intr_deleg_S(EX_IPF)){
          vstval->val = vaddr;
        }else{
          mtval->val = vaddr;
        }
        longjmp_exception(EX_IPF);
      }else{
        INTR_TVAL_REG(EX_IPF) = vaddr;
        longjmp_exception(EX_IPF);
      }
#else
      INTR_TVAL_REG(EX_IPF) = vaddr;
      longjmp_exception(EX_IPF);
#endif
    } else if(type == MEM_TYPE_READ){
#ifdef CONFIG_RVH
      int ex;
      if(hld_st || gpf){
        ex = cpu.amo ? EX_SGPF : EX_LGPF;
        if(intr_deleg_S(ex)){
          stval->val = vaddr;
          htval->val = vaddr >> 2;
        }else{
          mtval->val = vaddr;
          mtval2->val = vaddr >> 2;
        }
      }else if(cpu.v){
        ex = cpu.amo ? EX_SPF : EX_LPF;
        if(intr_deleg_S(ex)){
          vstval->val = vaddr;
        }else{
          mtval->val = vaddr;
        }
      }else{
        ex = cpu.amo ? EX_SPF : EX_LPF;
        INTR_TVAL_REG(ex) = vaddr;
      }
      longjmp_exception(ex);
#else
      int ex = cpu.amo ? EX_SPF : EX_LPF;
      INTR_TVAL_REG(ex) = vaddr;
      longjmp_exception(ex);
#endif
    } else {
#ifdef CONFIG_RVH
      if(hld_st || gpf){
        if(intr_deleg_S(EX_SGPF)){
          stval->val = vaddr;
          htval->val = vaddr >> 2;
        }else{
          mtval->val = vaddr;
          mtval2->val = vaddr >> 2;
        }
        longjmp_exception(EX_SGPF);
      }else if(cpu.v){
        if(intr_deleg_S(EX_SPF)){
          vstval->val = vaddr;
        }else{
          mtval->val = vaddr;
        }
        longjmp_exception(EX_SPF);
      }else{
        INTR_TVAL_REG(EX_SPF) = vaddr;
        longjmp_exception(EX_SPF);
      }
#else
      INTR_TVAL_REG(EX_SPF) = vaddr;
      longjmp_exception(EX_SPF);
#endif
    }
    return MEM_RET_FAIL;
  }
#ifdef CONFIG_RVH
  if (cpu.v && is_ifetch) return h_mmu_state ? MMU_TRANSLATE : MMU_DIRECT;
#endif
  if (is_ifetch) return ifetch_mmu_state ? MMU_TRANSLATE : MMU_DIRECT;
#ifdef CONFIG_RVH
  if(hld_st)
    return h_mmu_state  ? MMU_TRANSLATE : MMU_DIRECT;
#endif
  return data_mmu_state ? MMU_TRANSLATE : MMU_DIRECT;
}

void isa_misalign_data_addr_check(vaddr_t vaddr, int len, int type) {
  if (ISDEF(CONFIG_AC_SOFT) && unlikely((vaddr & (len - 1)) != 0)) {
    Log("addr misaligned happened: vaddr:%lx len:%d type:%d pc:%lx", vaddr, len, type, cpu.pc);
    int ex = cpu.amo || type == MEM_TYPE_WRITE ? EX_SAM : EX_LAM;
    INTR_TVAL_REG(ex) = vaddr;
    longjmp_exception(ex);
  }
}

paddr_t isa_mmu_translate(vaddr_t vaddr, int len, int type) {
  paddr_t ptw_result = ptw(vaddr, type);
#ifdef FORCE_RAISE_PF
  if(ptw_result != MEM_RET_FAIL && force_raise_pf(vaddr, type) != MEM_RET_OK)
    return MEM_RET_FAIL;
#endif
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
      if (!intr_deleg_S(EX_IPF)) {
        mtval->val = cpu.execution_guide.mtval;
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
      } else {
        stval->val = cpu.execution_guide.stval;
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
      }
      printf("force raise IPF\n");
      longjmp_exception(EX_IPF);
      return MEM_RET_FAIL;
    } else if(!ifetch && type == MEM_TYPE_READ && cpu.execution_guide.exception_num == EX_LPF){
      if (force_raise_pf_record(vaddr, type)) {
        return MEM_RET_OK;
      }
      INTR_TVAL_REG(EX_LPF) = vaddr;
      printf("force raise LPF\n");
      longjmp_exception(EX_LPF);
      return MEM_RET_FAIL;
    } else if(type == MEM_TYPE_WRITE && cpu.execution_guide.exception_num == EX_SPF){
      if (force_raise_pf_record(vaddr, type)) {
        return MEM_RET_OK;
      }
      INTR_TVAL_REG(EX_SPF) = vaddr;
      printf("force raise SPF\n");
      longjmp_exception(EX_SPF);
      return MEM_RET_FAIL;
    }
  }
  return MEM_RET_OK;
}

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
  if (CONFIG_RV_PMP_NUM == 0) {
    return true;
  }

  word_t base = 0;
  for (int i = 0; i < CONFIG_RV_PMP_NUM; i++) {
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
  if (CONFIG_RV_PMP_NUM == 0) {
    return true;
  }
  int i = 0;
  word_t base = 0;
  for (i = 0; i < CONFIG_RV_PMP_NUM; i++) {
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
