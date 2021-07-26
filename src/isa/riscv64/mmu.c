#include <isa.h>
#include <memory/vaddr.h>
#include <memory/paddr.h>
#include "local-include/csr.h"
#include "local-include/intr.h"
#include <stdlib.h>

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
  ok = ok && pte->v;
  ok = ok && !(mode == MODE_U && !pte->u);
  ok = ok && !(pte->u && ((mode == MODE_S) && (!mstatus->sum || ifetch)));
  if (ifetch) {
    if (!(ok && pte->x)) {
      assert(!cpu.amo);
      stval->val = vaddr;
      cpu.mem_exception = EX_IPF;
      return false;
    }
  } else if (type == MEM_TYPE_READ) {
    bool can_load = pte->r || (mstatus->mxr && pte->x);
    if (!(ok && can_load)) {
      if (cpu.mode == MODE_M) mtval->val = vaddr;
      else stval->val = vaddr;
      if (cpu.amo) Log("redirect to AMO page fault exception at pc = " FMT_WORD, cpu.pc);
      cpu.mem_exception = (cpu.amo ? EX_SPF : EX_LPF);
      return false;
    }
  } else {
    if (!(ok && pte->w)) {
      if (cpu.mode == MODE_M) mtval->val = vaddr;
      else stval->val = vaddr;
      cpu.mem_exception = EX_SPF;
      return false;
    }
  }
  return true;
}

static int get_page_size(uint64_t vaddr) {
  int level;
  PTE pte;
  word_t pg_base = PGBASE(satp->ppn);
  word_t p_pte;

  for (level = PTW_LEVEL - 1; level >= 0; ) {
    p_pte = pg_base + VPNi(vaddr, level) * PTE_SIZE;
    pte.val = paddr_read(p_pte, PTE_SIZE);
    pg_base = PGBASE(pte.ppn);
    if (!pte.v) {
      break;
    }
    if (pte.r || pte.x) { break; }
    else {
      level --;
      if (level < 0) { break; }
    }
  }
  if (level < 0 || !pte.v) return -1;
  if (level == 0) return PAGE_4KB;
  if (level == 1) return PAGE_2MB;
  else return PAGE_1GB;
}

bool inline normal_page_hit(tlb_entry *entry, uint64_t tag) {
  return entry->v && entry->tag == tag;
}

bool inline super_page_hit(tlb_sp_entry *entry, uint64_t vaddr) {
  return entry->v && (entry->size == PAGE_2MB ? SUPERVPN(entry->tag, 2) == SUPERVPN(vaddr, 2) : SUPERVPN(entry->tag, 1) == SUPERVPN(vaddr, 1));
}

bool tlb_l1_access(uint64_t vaddr, uint64_t type) {
  riscv64_TLB_State *tlb;
  if (type == MEM_TYPE_IFETCH) { tlb = &itlb; }
  else { tlb = &dtlb; }

  tlb->access += 1;
  int size = get_page_size(vaddr);
  if (size < 0) return true; // return true to aviod more actions

  for (int i = 0; i < TLBEntryNum && size == PAGE_4KB; i++) {
    if (normal_page_hit(&tlb->normal[i], VPN(vaddr))) {
      return true;
    }
  }
  for (int i = 0; i < TLBSPEntryNum && size != PAGE_4KB; i++) {
    if (super_page_hit(&tlb->super[i], vaddr)) {
      return true;
    }
  }

  // miss, then refill
  tlb->miss ++;
  if (size == PAGE_4KB) {
    int refill_index = rand() % TLBEntryNum;
    tlb->normal[refill_index].v = true;
    tlb->normal[refill_index].tag = VPN(vaddr);
  } else {
    int refill_index = rand() % TLBSPEntryNum;
    tlb->super[refill_index].v = true;
    tlb->super[refill_index].tag = vaddr;
    tlb->super[refill_index].size = size;
  }

  return false;
}

bool l2tlb_l3_access(uint64_t vaddr) {
  l2tlb.access ++;
  int size = get_page_size(vaddr);
  if (size < 0) return true;

  int index = get_l3_index(vaddr);
  for (int i = 0; i < L2TLBL3WayNum && size == PAGE_4KB; i++) {
    if (normal_page_hit(&l2tlb.l3[index][i], get_l3_tag(vaddr))) {
      return true;
    }
  }
  for (int i = 0; i < L2TLBSPEntryNum && size != PAGE_4KB; i ++) {
    if (super_page_hit(&l2tlb.sp[i], vaddr)){
      return true;
    }
  }

  // miss, then refill
  l2tlb.miss ++;
  l2tlb.mem_access ++;
  if (size == PAGE_4KB) {
    int refill_index = rand() % L2TLBL3WayNum;
    l2tlb.l3[index][refill_index].tag = get_l3_tag(vaddr);
    l2tlb.l3[index][refill_index].v = true;
  } else {
    int refill_index = rand() % L2TLBSPEntryNum;
    l2tlb.sp[refill_index].tag = vaddr;
    l2tlb.sp[refill_index].v = true;
  }

  return false;
}

bool l2tlb_l2_access(uint64_t vaddr) {
  int size = get_page_size(vaddr);
  if (size < 0) return true;

  int index = get_l2_index(vaddr);
  for (int i = 0; i < L2TLBL2WayNum && size == PAGE_4KB; i ++) {
    if (normal_page_hit(&l2tlb.l2[index][i], get_l2_tag(vaddr))) {
      return true;
    }
  }

  // miss, then refill
  if (size == PAGE_4KB) {
    l2tlb.mem_access ++;
    int refill_index = rand() % L2TLBL2WayNum;
    l2tlb.l2[index][refill_index].tag = get_l2_tag(vaddr);
    l2tlb.l2[index][refill_index].v = true;
  }

  return false;
}

bool l2tlb_l1_access(uint64_t vaddr) {
  int size = get_page_size(vaddr);
  if (size < 0) return true;

  for (int i = 0; i < L2TLBL1EntryNum && (size == PAGE_4KB || size == PAGE_2MB); i++) {
    if (normal_page_hit(&l2tlb.l1[i], get_l2_tag(vaddr))) {
      return true;
    }
  }

  if (size == PAGE_4KB || size == PAGE_2MB) {
    l2tlb.mem_access ++;
    int refill_index = rand() % L2TLBL1EntryNum;
    l2tlb.l1[refill_index].tag = get_l1_tag(vaddr);
    l2tlb.l1[refill_index].v = true;
  }

  return false;
}

void riscv64_tlb_access(uint64_t vaddr, uint64_t type) {
  if (!tlb_l1_access(vaddr, type))
    if (!l2tlb_l3_access(vaddr))
      if (!l2tlb_l2_access(vaddr))
        l2tlb_l1_access(vaddr);
}

static paddr_t ptw(vaddr_t vaddr, int type) {

  isa_tlb_access(vaddr, type);

  word_t pg_base = PGBASE(satp->ppn);
  word_t p_pte; // pte pointer
  PTE pte;
  int level;
  for (level = PTW_LEVEL - 1; level >= 0;) {
    p_pte = pg_base + VPNi(vaddr, level) * PTE_SIZE;
    pte.val	= paddr_read(p_pte, PTE_SIZE);
#ifdef XIANGSHAN_DEBUG
    Log("[NEMU] ptw: level %d, vaddr 0x%lx, pg_base 0x%lx, p_pte 0x%lx, pte.val 0x%lx\n",
      level, vaddr, pg_base, p_pte, pte.val);
#endif
    pg_base = PGBASE(pte.ppn);
    if (!pte.v) {
//      Log("level %d: pc = " FMT_WORD ", vaddr = " FMT_WORD
//          ", pg_base = " FMT_WORD ", p_pte = " FMT_WORD ", pte = " FMT_WORD,
//          level, cpu.pc, vaddr, pg_base, p_pte, pte.val);
      break;
    }
    if (pte.r || pte.x) { break; }
    else {
      level --;
      if (level < 0) { if (!check_permission(&pte, false, vaddr, type)) return MEM_RET_FAIL; }
    }
  }

  if (!check_permission(&pte, true, vaddr, type)) return MEM_RET_FAIL;

  if (level > 0) {
    // superpage
    word_t pg_mask = ((1ull << VPNiSHFT(level)) - 1);
    if ((pg_base & pg_mask) != 0) {
      // missaligned superpage
      if (!check_permission(&pte, false, vaddr, type)) return MEM_RET_FAIL;
    }
    pg_base = (pg_base & ~pg_mask) | (vaddr & pg_mask & ~PGMASK);
  }

#if !_SHARE
  if (!isa_has_mem_exception()) {
    bool is_write = (type == MEM_TYPE_WRITE);
    if (!pte.a || (!pte.d && is_write)) {
      pte.a = true;
      pte.d |= is_write;
      paddr_write(p_pte, pte.val, PTE_SIZE);
    }
  }
#endif

  return isa_has_mem_exception() ? MEM_RET_FAIL : pg_base | MEM_RET_OK;
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

  if(cpu.need_disambiguate){
    if(ifetch && cpu.disambiguation_state.exceptionNo == EX_IPF){
      if (force_raise_pf_record(vaddr, type)) {
        return MEM_RET_OK;
      }
      if (cpu.mode == MODE_M) {
        mtval->val = cpu.disambiguation_state.mtval;
        if(
          vaddr != cpu.disambiguation_state.mtval &&
          // cross page ipf caused mismatch is legal
          !((vaddr & 0xfff) == 0xffe && (cpu.disambiguation_state.mtval & 0xfff) == 0x000)
        ){
          printf("[WRANING] nemu mtval %lx does not match core mtval %lx\n",
            vaddr,
            cpu.disambiguation_state.mtval
          );
        }
      } else {
        stval->val = cpu.disambiguation_state.stval;
        if(
          vaddr != cpu.disambiguation_state.stval &&
          // cross page ipf caused mismatch is legal
          !((vaddr & 0xfff) == 0xffe && (cpu.disambiguation_state.stval & 0xfff) == 0x000)
        ){
          printf("[WRANING] nemu stval %lx does not match core stval %lx\n",
            vaddr,
            cpu.disambiguation_state.stval
          );
        }
      }
      cpu.mem_exception = EX_IPF;
      printf("force raise IPF\n");
      return MEM_RET_FAIL;
    } else if(!ifetch && type == MEM_TYPE_READ && cpu.disambiguation_state.exceptionNo == EX_LPF){
      if (force_raise_pf_record(vaddr, type)) {
        return MEM_RET_OK;
      }
      if (cpu.mode == MODE_M) mtval->val = vaddr;
      else stval->val = vaddr;
      cpu.mem_exception = EX_LPF;
      printf("force raise LPF\n");
      return MEM_RET_FAIL;
    } else if(type == MEM_TYPE_WRITE && cpu.disambiguation_state.exceptionNo == EX_SPF){
      if (force_raise_pf_record(vaddr, type)) {
        return MEM_RET_OK;
      }
      if (cpu.mode == MODE_M) mtval->val = vaddr;
      else stval->val = vaddr;
      cpu.mem_exception = EX_SPF;
      printf("force raise SPF\n");
      return MEM_RET_FAIL;
    }
  }
  return MEM_RET_OK;
}

int isa_vaddr_check(vaddr_t vaddr, int type, int len) {

  bool ifetch = (type == MEM_TYPE_IFETCH);

  // riscv-privileged 4.4.1: Addressing and Memory Protection:
  // Instruction fetch addresses and load and store effective addresses,
  // which are 64 bits, must have bits 63–39 all equal to bit 38, or else a page-fault exception will occur.
  bool vm_enable = (mstatus->mprv && (!ifetch) ? mstatus->mpp : cpu.mode) < MODE_M && satp->mode == 8;
  word_t va_mask = ((((word_t)1) << (63 - 38 + 1)) - 1);
  word_t va_msbs = vaddr >> 38;
  bool va_msbs_ok = (va_msbs == va_mask) || va_msbs == 0 || !vm_enable;

// #ifdef FORCE_RAISE_PF
//   int forced_result = force_raise_pf(vaddr, type);
//   if(forced_result != MEM_RET_OK)
//     return forced_result;
// #endif

  if(!va_msbs_ok){
    if(ifetch){
      stval->val = vaddr;
      cpu.mem_exception = EX_IPF;
    } else if(type == MEM_TYPE_READ){
      if (cpu.mode == MODE_M) mtval->val = vaddr;
      else stval->val = vaddr;
      cpu.mem_exception = (cpu.amo ? EX_SPF : EX_LPF);
    } else {
      if (cpu.mode == MODE_M) mtval->val = vaddr;
      else stval->val = vaddr;
      cpu.mem_exception = EX_SPF;
    }
    return MEM_RET_FAIL;
  }


  if ((!ifetch) && (vaddr & (len - 1)) != 0) {
    mtval->val = vaddr;
    cpu.mem_exception = (cpu.amo || type == MEM_TYPE_WRITE ? EX_SAM : EX_LAM);
    // return MEM_RET_FAIL; // LAM|SAM, go on to check pf
  }

  uint32_t mode = (mstatus->mprv && (!ifetch) ? mstatus->mpp : cpu.mode);
  if (mode < MODE_M) {
    assert(satp->mode == 0 || satp->mode == 8);
    if (satp->mode == 8){
#ifdef ENABLE_DISAMBIGUATE
      if(!isa_mmu_safe(vaddr, type)){
        int forced_result = force_raise_pf(vaddr, type);
        if(forced_result != MEM_RET_OK)
          return forced_result;
      }
#endif
      return MEM_RET_NEED_TRANSLATE;
    }
  }
  return isa_has_mem_exception() ? MEM_RET_FAIL : MEM_RET_OK;
}

#ifdef ENABLE_DISAMBIGUATE
// Check if pte has been sfenced
//
// In several cases, there are mutliple legal control flows.
// e.g. pte may be still in sbuffer before it is used if sfence is not execuated
bool ptw_is_safe(vaddr_t vaddr) {
#ifdef ISA64
  int rsize = 8;
#else
  int rsize = 4;
#endif
  word_t pg_base = PGBASE(satp->ppn);
  word_t p_pte; // pte pointer
  PTE pte;
  int level;
  for (level = PTW_LEVEL - 1; level >= 0;) {
    p_pte = pg_base + VPNi(vaddr, level) * PTE_SIZE;
    pte.val	= paddr_read(p_pte, PTE_SIZE);
    if(!is_sfence_safe(p_pte, rsize)){
      // printf("[Warning] pte at %lx is not sfence safe, accessed by pc %lx\n", p_pte, cpu.pc);
      return false;
    }
    pg_base = PGBASE(pte.ppn);
    if (!pte.v) {
      //Log("level %d: pc = " FMT_WORD ", vaddr = " FMT_WORD
      //    ", pg_base = " FMT_WORD ", p_pte = " FMT_WORD ", pte = " FMT_WORD,
      //    level, cpu.pc, vaddr, pg_base, p_pte, pte.val);
      break;
    }
    if (pte.r || pte.x) { break; }
    else {
      level --;
      if (level < 0)
        return true;
    }
  }

  return true;
}
#endif

paddr_t isa_mmu_translate(vaddr_t vaddr, int type, int len) {
  paddr_t ptw_result = ptw(vaddr, type);
#ifdef FORCE_RAISE_PF
  if(ptw_result != MEM_RET_FAIL && force_raise_pf(vaddr, type) != MEM_RET_OK)
    return MEM_RET_FAIL;
#endif
  return ptw_result;
}

#ifdef ENABLE_DISAMBIGUATE
bool isa_mmu_safe(vaddr_t vaddr, int type) {
  bool ifetch = (type == MEM_TYPE_IFETCH);
  uint32_t mode = (mstatus->mprv && (!ifetch) ? mstatus->mpp : cpu.mode);
  if(mode < MODE_M && satp->mode == 8)
    return ptw_is_safe(vaddr);
  return true;
}
#endif

