#include "nemu.h"
#include "memory/memory.h"
#include "csr.h"
#include "intr.h"

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

static inline void check_permission(PTE *pte, bool ok, vaddr_t vaddr, bool is_write) {
  ok = ok && pte->v;
  ok = ok && !(cpu.mode == MODE_U && !pte->u);
  ok = ok && !(cpu.mode == MODE_S && pte->u && !mstatus->sum);
  if (cpu.fetching) {
    if (!(ok && pte->x)) {
      assert(!cpu.amo);
      stval->val = vaddr;
      longjmp_raise_intr(EX_IPF);
    }
  } else if (!is_write) {
    bool can_load = pte->r || (mstatus->mxr && pte->x);
    if (!(ok && can_load)) {
      stval->val = vaddr;
      if (cpu.amo) {
        cpu.amo = false;
        Log("redirect to AMO page fault exception at pc = " FMT_WORD, cpu.pc);
        longjmp_raise_intr(EX_SPF);
      }
      longjmp_raise_intr(EX_LPF);
    }
  } else {
    if (!(ok && pte->w)) {
      stval->val = vaddr;
      if (cpu.amo) cpu.amo = false;
      longjmp_raise_intr(EX_SPF);
    }
  }
}

static word_t page_walk(vaddr_t vaddr, bool is_write) {
  word_t pg_base = PGBASE(satp->ppn);
  word_t p_pte; // pte pointer
  PTE pte;
  int level;
  for (level = PTW_LEVEL - 1; level >= 0;) {
    p_pte = pg_base + VPNi(vaddr, level) * PTE_SIZE;
    pte.val	= paddr_read(p_pte, PTE_SIZE);
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
      if (level < 0) { check_permission(&pte, false, vaddr, is_write); }
    }
  }

  check_permission(&pte, true, vaddr, is_write);

  if (level > 0) {
    // superpage
    word_t pg_mask = ((1ull << VPNiSHFT(level)) - 1);
    if ((pg_base & pg_mask) != 0) {
      // missaligned superpage
      check_permission(&pte, false, vaddr, is_write);
    }
    pg_base = (pg_base & ~pg_mask) | (vaddr & pg_mask & ~PGMASK);
  }

  if (!pte.a || (!pte.d && is_write)) {
    pte.a = true;
    pte.d |= is_write;
    paddr_write(p_pte, pte.val, PTE_SIZE);
  }

  return pg_base;
}

static inline paddr_t page_translate(vaddr_t addr, bool is_write) {
  return page_walk(addr, is_write) | (addr & PAGE_MASK);
}

word_t isa_vaddr_read(vaddr_t addr, int len) {
  paddr_t paddr = addr;
  uint32_t mode = (mstatus->mprv && !cpu.fetching ? mstatus->mpp : cpu.mode);
  if (mode < MODE_M) {
    assert(satp->mode == 0 || satp->mode == 8);
    if (satp->mode == 8) {
      paddr = page_translate(addr, false);
    }
  }
  return paddr_read(paddr, len);
}

void isa_vaddr_write(vaddr_t addr, word_t data, int len) {
  paddr_t paddr = addr;
  uint32_t mode = (mstatus->mprv && !cpu.fetching ? mstatus->mpp : cpu.mode);
  if (mode < MODE_M) {
    assert(satp->mode == 0 || satp->mode == 8);
    if (satp->mode == 8) {
      paddr = page_translate(addr, true);
    }
  }
  paddr_write(paddr, data, len);
}
