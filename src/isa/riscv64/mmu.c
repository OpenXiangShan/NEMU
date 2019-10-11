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

typedef union {
  struct {
    uint64_t pf_off		:12;
    uint64_t vpn0  		: 9;
    uint64_t vpn1  		: 9;
    uint64_t vpn2  		: 9;
  };
  uint64_t addr;
} PageAddr;

// Sv39 page walk
static word_t page_walk(vaddr_t vaddr, bool is_write) {
  PageAddr *addr = (void *)&vaddr;
  word_t pdir_base = satp->ppn << 12;

  PTE pte2;
  pte2.val	= paddr_read(pdir_base + addr->vpn2 * 8, 8);
  if (!pte2.valid) {
    panic("pc = " FMT_WORD ", vaddr = " FMT_WORD ", pdir_base = " FMT_WORD ", pte2 = " FMT_WORD,
        cpu.pc, vaddr, pdir_base, pte2.val);
  }

  word_t pt_base1 = pte2.ppn << 12;
  PTE pte1;
  pte1.val = paddr_read(pt_base1 + addr->vpn1 * 8, 8);
  if (!pte1.valid) {
    panic("pc = " FMT_WORD ", vaddr = " FMT_WORD ", pt_base1 = " FMT_WORD ", pte1 = " FMT_WORD,
      cpu.pc, vaddr, pt_base1, pte1.val);
  }

  word_t pt_base0 = pte1.ppn << 12;
  PTE pte0;
  pte0.val = paddr_read(pt_base0 + addr->vpn0 * 8, 8);
  if (!pte0.valid) {
    panic("pc = " FMT_WORD ", vaddr = " FMT_WORD ", pt_base0 = " FMT_WORD ", pte0 = " FMT_WORD,
      cpu.pc, vaddr, pt_base0, pte0.val);
  }

  //if (!pte.access || (pte.dirty == 0 && is_write)) {
  //  pte.access = 1;
  //  pte.dirty |= is_write;
  //  paddr_write(pt_base + addr->pt_idx * 4, pte.val, 4);
  //}

  return pte0.ppn << 12;
}

static inline paddr_t page_translate(vaddr_t addr, bool is_write) {
  return page_walk(addr, is_write) | (addr & PAGE_MASK);
}

word_t isa_vaddr_read(vaddr_t addr, int len) {
  assert(satp->mode == 0 || satp->mode == 4);
  paddr_t paddr = (satp->mode == 4 ? page_translate(addr, false) : addr);
  return paddr_read(paddr, len);
}

void isa_vaddr_write(vaddr_t addr, word_t data, int len) {
  assert(satp->mode == 0 || satp->mode == 4);
  paddr_t paddr = (satp->mode == 4 ? page_translate(addr, true) : addr);
  paddr_write(paddr, data, len);
}
