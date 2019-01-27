#include "nemu.h"
#include "memory/memory.h"
#include "isa/intr.h"
#include <stdlib.h>
#include <time.h>

#define NR_TLB 128

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

void init_mmu(void) {
  int i;
  for (i = 0; i < NR_TLB; i ++) {
    tlb[i].lo[0].V = tlb[i].lo[1].V = 0;
  }
  srand(time(0));
}

void update_tlb(int idx) {
  tlb[idx].hi.val = cpu.entryhi.val;
  tlb[idx].lo[0].val = cpu.entrylo0;
  tlb[idx].lo[1].val = cpu.entrylo1;
}

void tlbwr(void) {
  update_tlb(rand() % NR_TLB);
}

void tlbwi(void) {
  update_tlb(cpu.index % NR_TLB);
}

void tlbp(void) {
  int i;
  for (i = 0; i < NR_TLB; i ++) {
    if (tlb[i].hi.VPN2 == cpu.entryhi.VPN2) {
      Log("match, i = %d", i);
      cpu.index = i;
      return;
    }
  }
  cpu.index = -1;
}

extern void longjmp_raise_intr(uint8_t NO);

static inline int32_t search_ppn(vaddr_t addr, bool write) {
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
        longjmp_raise_intr(write ? EX_TLB_ST : EX_TLB_LD);
        return -1;
      }
      //Assert(tlb[i].lo[a.lo_idx].V, "cpu.pc = 0x%08x, addr = 0x%08x, lo0 = 0x%08x, lo1 = 0x%08x",
      //    cpu.pc, addr, tlb[i].lo[0].val, tlb[i].lo[1].val);
      return tlb[i].lo[a.lo_idx].PFN;
    }
  }
  cpu.entryhi.VPN2 = a.vpn;
//  Log("tlb refill at cpu.pc = 0x%08x, badaddr = 0x%08x", cpu.pc, addr);
  longjmp_raise_intr(TLB_REFILL | (write ? EX_TLB_ST : EX_TLB_LD));
  return -1;
}

static inline paddr_t va2pa(vaddr_t addr, bool write) {
  if ((addr & 0xa0000000u) == 0xa0000000u) {
    return addr & ~0xa0000000u;
  }

  if ((addr & 0x80000000u) == 0) {
    int32_t ppn = search_ppn(addr, write);
    addr = (addr & 0xfff) | (ppn << 12);
  }

  return addr & ~0x80000000u;
}

uint32_t isa_vaddr_read(vaddr_t addr, int len) {
  return paddr_read(va2pa(addr, false), len);
}

void isa_vaddr_write(vaddr_t addr, uint32_t data, int len) {
  paddr_write(va2pa(addr, true), data, len);
}
