#include "nemu.h"
#include "memory/memory.h"
#include "csr.h"

typedef union PageTableEntry {
  struct {
    uint32_t valid  : 1;
    uint32_t read   : 1;
    uint32_t write  : 1;
    uint32_t exec   : 1;
    uint32_t user   : 1;
    uint32_t global : 1;
    uint32_t access : 1;
    uint32_t dirty  : 1;
    uint32_t rsw    : 2;
    uint64_t ppn    :44;
    uint32_t pad    :10;
  };
  uint64_t val;
} PTE;

#define PGSHFT 12
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

static word_t page_walk(vaddr_t vaddr, bool is_write) {
  word_t pg_base = PGBASE(satp->ppn);
  PTE pte;
  int level;
  for (level = PTW_LEVEL - 1; level >= 0; level --) {
    pte.val	= paddr_read(pg_base + VPNi(vaddr, level) * PTE_SIZE, PTE_SIZE);
    if (!pte.valid) {
      panic("level %d: pc = " FMT_WORD ", vaddr = " FMT_WORD ", pg_base = " FMT_WORD ", pte = " FMT_WORD,
          level, cpu.pc, vaddr, pg_base, pte.val);
    }
    pg_base = PGBASE(pte.ppn);
  }

  //if (!pte.access || (pte.dirty == 0 && is_write)) {
  //  pte.access = 1;
  //  pte.dirty |= is_write;
  //  paddr_write(pt_base + addr->pt_idx * 4, pte.val, 4);
  //}

  return pg_base;
}

static inline paddr_t page_translate(vaddr_t addr, bool is_write) {
  return page_walk(addr, is_write) | (addr & PAGE_MASK);
}

word_t isa_vaddr_read(vaddr_t addr, int len) {
  assert(satp->mode == 0 || satp->mode == 8);
  paddr_t paddr = (satp->mode == 8 ? page_translate(addr, false) : addr);
  return paddr_read(paddr, len);
}

void isa_vaddr_write(vaddr_t addr, word_t data, int len) {
  assert(satp->mode == 0 || satp->mode == 8);
  paddr_t paddr = (satp->mode == 8 ? page_translate(addr, true) : addr);
  paddr_write(paddr, data, len);
}
