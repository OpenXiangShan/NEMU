#include <isa.h>
#include <memory/paddr.h>
#include <memory/vaddr.h>
#ifndef __ICS_EXPORT
#include <cpu/cpu.h>
#include "../local-include/intr.h"
#include <stdlib.h>
#include <time.h>

#define NR_TLB 16

typedef union {
  struct {
    uint32_t ASID: 8;
    uint32_t pad : 5;
    uint32_t VPN2:19;
  };
  uint32_t val;
} EntryHi;

typedef union {
  struct {
    uint32_t G   : 1;
    uint32_t V   : 1;
    uint32_t D   : 1;
    uint32_t C   : 3;
    uint32_t PFN :24;
    uint32_t pad : 2;
  };
  uint32_t val;
} EntryLo;

struct {
  EntryHi hi;
  EntryLo lo[2];
} tlb [NR_TLB];

void init_mmu() {
  int i;
  for (i = 0; i < NR_TLB; i ++) {
    tlb[i].lo[0].V = tlb[i].lo[1].V = 0;
  }
  srand(time(0));
}

static inline void update_tlb(int idx) {
  tlb[idx].hi.val = cpu.entryhi.val;
  tlb[idx].lo[0].val = cpu.entrylo0;
  tlb[idx].lo[1].val = cpu.entrylo1;
}

void tlbwr() {
  update_tlb(rand() % NR_TLB);
}

void tlbwi() {
  update_tlb(cpu.index % NR_TLB);
}

void tlbp() {
  int i;
  for (i = 0; i < NR_TLB; i ++) {
    if (tlb[i].hi.VPN2 == cpu.entryhi.VPN2) {
      Log("match, i = %d, cpu.pc = 0x%08x, va = 0x%08x", i, cpu.pc, cpu.entryhi.val);
      cpu.index = i;
      return;
    }
  }
  cpu.index |= 0x80000000;
}

static inline int32_t search_ppn(vaddr_t addr, int type) {
  union {
    struct {
      uint32_t offset :12;
      uint32_t lo_idx : 1;
      uint32_t vpn    :19;
    };
    uint32_t val;
  } a;
  a.val = addr;
  int i;
  for (i = 0; i < NR_TLB; i ++) {
    if (tlb[i].hi.VPN2 == a.vpn) {
      if (!tlb[i].lo[a.lo_idx].V) {
        cpu.entryhi.VPN2 = a.vpn;
//        Log("tlb[%d] invalid at cpu.pc = 0x%08x, badaddr = 0x%08x", i, cpu.pc, addr);
        longjmp_exec(type == MEM_TYPE_WRITE ? EX_TLB_ST : EX_TLB_LD);
      }
      //Assert(tlb[i].lo[a.lo_idx].V, "cpu.pc = 0x%08x, addr = 0x%08x, lo0 = 0x%08x, lo1 = 0x%08x",
      //    cpu.pc, addr, tlb[i].lo[0].val, tlb[i].lo[1].val);
      return tlb[i].lo[a.lo_idx].PFN;
    }
  }
  cpu.entryhi.VPN2 = a.vpn;
//  Log("tlb refill at cpu.pc = 0x%08x, badaddr = 0x%08x", cpu.pc, addr);
  longjmp_exec(TLB_REFILL | (type == MEM_TYPE_WRITE ? EX_TLB_ST : EX_TLB_LD));
  return -1;
}
#endif

paddr_t isa_mmu_translate(vaddr_t vaddr, int len, int type) {
#ifdef __ICS_EXPORT
  return MEM_RET_FAIL;
#else
  int32_t ppn = search_ppn(vaddr, type);
  if (ppn == -1) return MEM_RET_FAIL;
  return ((uint32_t)(ppn << 12) + 0x80000000) | MEM_RET_OK;
#endif
}
