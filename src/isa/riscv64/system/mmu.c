#include <isa.h>
#include <memory/vaddr.h>
#include <memory/paddr.h>
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
static inline uintptr_t VPNiSHFT(int i) {
  return (PGSHFT) + 9 * i;
}
static inline uintptr_t VPNi(vaddr_t va, int i) {
  return (va >> VPNiSHFT(i)) & VPNMASK;
}

static inline bool check_permission(PTE *pte, bool ok, vaddr_t vaddr, int type) {
  bool ifetch = (type == MEM_TYPE_IFETCH);
  uint32_t mode = (mstatus->mprv && !ifetch ? mstatus->mpp : cpu.mode);
  assert(mode == MODE_U || mode == MODE_S);
  ok = ok && pte->v;
  ok = ok && !(mode == MODE_U && !pte->u);
  ok = ok && !(pte->u && ((mode == MODE_S) && (!mstatus->sum || ifetch)));
  if (ifetch) {
#ifdef CONFIG_SHARE
//  update a/d by exception
    bool update_ad = !pte->a;
//   if (update_ad && ok && pte->x) Log("raise exception to update ad for ifecth");
#else
    bool update_ad = false;
#endif
    if (!(ok && pte->x) || update_ad) {
      assert(!cpu.amo);
      stval->val = vaddr;
      longjmp_exception(EX_IPF);
      return false;
    }
  } else if (type == MEM_TYPE_READ) {
    bool can_load = pte->r || (mstatus->mxr && pte->x);
#ifdef CONFIG_SHARE
    bool update_ad = !pte->a;
//    if (update_ad && ok && can_load) Log("raise exception to update ad for load");
#else
    bool update_ad = false;
#endif
    if (!(ok && can_load) || update_ad) {
      if (cpu.mode == MODE_M) mtval->val = vaddr;
      else stval->val = vaddr;
      //if (cpu.amo) Log("redirect to AMO page fault exception at pc = " FMT_WORD, cpu.pc);
      int ex = (cpu.amo ? EX_SPF : EX_LPF);
      cpu.amo = false;
      longjmp_exception(ex);
      return false;
    }
  } else {
#ifdef CONFIG_SHARE
    bool update_ad = !pte->a || !pte->d;
//    if (update_ad && ok && pte->w) Log("raise exception to update ad for store");
#else
    bool update_ad = false;
#endif
    if (!(ok && pte->w) || update_ad) {
      if (cpu.mode == MODE_M) mtval->val = vaddr;
      else stval->val = vaddr;
      cpu.amo = false;
      longjmp_exception(EX_SPF);
      return false;
    }
  }
  return true;
}

static paddr_t ptw(vaddr_t vaddr, int type) {

  word_t pg_base = PGBASE(satp->ppn);
  word_t p_pte; // pte pointer
  PTE pte;
  int level;
  int64_t vaddr39 = vaddr << (64 - 39);
  vaddr39 >>= (64 - 39);
  if ((uint64_t)vaddr39 != vaddr) goto bad;
  for (level = PTW_LEVEL - 1; level >= 0;) {
    p_pte = pg_base + VPNi(vaddr, level) * PTE_SIZE;
#ifdef CONFIG_MULTICORE_DIFF
    pte.val = golden_pmem_read(p_pte, PTE_SIZE);
#else
    pte.val	= paddr_read(p_pte, PTE_SIZE);
#endif
#ifdef XIANGSHAN_DEBUG
    printf("[NEMU] ptw: level %d, vaddr 0x%lx, pg_base 0x%lx, p_pte 0x%lx, pte.val 0x%lx\n",
      level, vaddr, pg_base, p_pte, pte.val);
#endif
    pg_base = PGBASE(pte.ppn);
    if (!pte.v || (!pte.r && pte.w)) goto bad;
    if (pte.r || pte.x) { break; }
    else {
      level --;
      if (level < 0) { goto bad; }
    }
  }

  if (!check_permission(&pte, true, vaddr, type)) return MEM_RET_FAIL;

  if (level > 0) {
    // superpage
    word_t pg_mask = ((1ull << VPNiSHFT(level)) - 1);
    if ((pg_base & pg_mask) != 0) {
      // missaligned superpage
      goto bad;
    }
    pg_base = (pg_base & ~pg_mask) | (vaddr & pg_mask & ~PGMASK);
  }

#ifndef CONFIG_SHARE
  // update a/d by hardware
  bool is_write = (type == MEM_TYPE_WRITE);
  if (!pte.a || (!pte.d && is_write)) {
    pte.a = true;
    pte.d |= is_write;
    paddr_write(p_pte, PTE_SIZE, pte.val);
  }
#endif

  return pg_base | MEM_RET_OK;

bad:
  check_permission(&pte, false, vaddr, type);
  return MEM_RET_FAIL;
}

static int ifetch_mmu_state = MMU_DIRECT;
static int data_mmu_state = MMU_DIRECT;

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
  return (data_mmu_state ^ data_mmu_state_old) ? true : false;
}

int isa_mmu_check(vaddr_t vaddr, int len, int type) {
  bool is_ifetch = type == MEM_TYPE_IFETCH;
  // riscv-privileged 4.4.1: Addressing and Memory Protection: 
  // Instruction fetch addresses and load and store effective addresses, 
  // which are 64 bits, must have bits 63–39 all equal to bit 38, or else a page-fault exception will occur.
  bool vm_enable = (mstatus->mprv && (!is_ifetch) ? mstatus->mpp : cpu.mode) < MODE_M && satp->mode == 8;
  word_t va_mask = ((((word_t)1) << (63 - 38 + 1)) - 1);
  word_t va_msbs = vaddr >> 38;
  bool va_msbs_ok = (va_msbs == va_mask) || va_msbs == 0 || !vm_enable;

  if(!va_msbs_ok){
    if(is_ifetch){
      stval->val = vaddr;
      longjmp_exception(EX_IPF);
    } else if(type == MEM_TYPE_READ){
      if (cpu.mode == MODE_M) mtval->val = vaddr;
      else stval->val = vaddr;
      longjmp_exception(cpu.amo ? EX_SPF : EX_LPF);
    } else {
      if (cpu.mode == MODE_M) mtval->val = vaddr;
      else stval->val = vaddr;
      longjmp_exception(EX_SPF);
    }
    return MEM_RET_FAIL;
  }

  if (is_ifetch) return ifetch_mmu_state ? MMU_TRANSLATE : MMU_DIRECT;
  if (ISDEF(CONFIG_AC_SOFT) && unlikely((vaddr & (len - 1)) != 0)) {
    Log("addr misaligned happened: vaddr:%lx len:%d type:%d pc:%lx", vaddr, len, type, cpu.pc);
    assert(0);
    mtval->val = vaddr;
    longjmp_exception(cpu.amo || type == MEM_TYPE_WRITE ? EX_SAM : EX_LAM);
    return MEM_RET_FAIL;
  }
  return data_mmu_state ? MMU_TRANSLATE : MMU_DIRECT;
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

  if(cpu.guided_exec){
    if(ifetch && cpu.execution_guide.exceptionNo == EX_IPF){
      if (force_raise_pf_record(vaddr, type)) {
        return MEM_RET_OK;
      }
      if (cpu.mode == MODE_M) {
        mtval->val = cpu.execution_guide.mtval;
        if(
          vaddr != cpu.execution_guide.mtval &&
          // cross page ipf caused mismatch is legal
          !((vaddr & 0xfff) == 0xffe && (cpu.execution_guide.mtval & 0xfff) == 0x000)
        ){
          printf("[WRANING] nemu mtval %lx does not match core mtval %lx\n",
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
          printf("[WRANING] nemu stval %lx does not match core stval %lx\n",
            vaddr,
            cpu.execution_guide.stval
          );
        }
      }
      printf("force raise IPF\n");
      longjmp_exception(EX_IPF);
      return MEM_RET_FAIL;
    } else if(!ifetch && type == MEM_TYPE_READ && cpu.execution_guide.exceptionNo == EX_LPF){
      if (force_raise_pf_record(vaddr, type)) {
        return MEM_RET_OK;
      }
      if (cpu.mode == MODE_M) mtval->val = vaddr;
      else stval->val = vaddr;
      printf("force raise LPF\n");
      longjmp_exception(EX_LPF);
      return MEM_RET_FAIL;
    } else if(type == MEM_TYPE_WRITE && cpu.execution_guide.exceptionNo == EX_SPF){
      if (force_raise_pf_record(vaddr, type)) {
        return MEM_RET_OK;
      }
      if (cpu.mode == MODE_M) mtval->val = vaddr;
      else stval->val = vaddr;
      printf("force raise SPF\n");
      longjmp_exception(EX_SPF);
      return MEM_RET_FAIL;
    }
  }
  return MEM_RET_OK;
}
