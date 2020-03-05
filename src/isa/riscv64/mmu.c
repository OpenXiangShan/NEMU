#include <isa.h>
#include <memory/memory.h>
#include "local-include/csr.h"
#include "local-include/intr.h"

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

static inline bool check_permission(PTE *pte, bool ok, vaddr_t vaddr, bool is_write) {
  uint32_t mode = (mstatus->mprv && !cpu.fetching ? mstatus->mpp : cpu.mode);
  ok = ok && pte->v;
  ok = ok && !(mode == MODE_U && !pte->u);
  ok = ok && !(pte->u && ((mode == MODE_S) && (!mstatus->sum || cpu.fetching)));
  if (cpu.fetching) {
    if (!(ok && pte->x)) {
      assert(!cpu.amo);
      stval->val = vaddr;
      cpu.mem_exception = EX_IPF;
      return false;
    }
  } else if (!is_write) {
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
      if (cpu.amo) cpu.amo = false;
      if (cpu.mode == MODE_M) mtval->val = vaddr;
      else stval->val = vaddr;
      cpu.mem_exception = EX_SPF;
      return false;
    }
  }
  return true;
}

static bool page_walk(vaddr_t vaddr, paddr_t *paddr, bool is_write) {
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
      if (level < 0) { if (!check_permission(&pte, false, vaddr, is_write)) return false; }
    }
  }

  if (!check_permission(&pte, true, vaddr, is_write)) return false;

  if (level > 0) {
    // superpage
    word_t pg_mask = ((1ull << VPNiSHFT(level)) - 1);
    if ((pg_base & pg_mask) != 0) {
      // missaligned superpage
      if (!check_permission(&pte, false, vaddr, is_write)) return false;
    }
    pg_base = (pg_base & ~pg_mask) | (vaddr & pg_mask & ~PGMASK);
  }

  if (!pte.a || (!pte.d && is_write)) {
    pte.a = true;
    pte.d |= is_write;
    paddr_write(p_pte, pte.val, PTE_SIZE);
  }

  *paddr = pg_base;
  return true;
}

static inline bool page_translate(vaddr_t vaddr, paddr_t *paddr, bool is_write) {
  uint32_t mode = (mstatus->mprv && !cpu.fetching ? mstatus->mpp : cpu.mode);
  if (mode < MODE_M) {
    assert(satp->mode == 0 || satp->mode == 8);
    if (satp->mode == 8) {
      if (!page_walk(vaddr, paddr, is_write)) return false;
      *paddr |= (vaddr & PAGE_MASK);
      return true;
    }
  }
  *paddr = vaddr;
  return true;
}

/*
word_t isa_vaddr_read(vaddr_t addr, int len) {
  if (!cpu.fetching) {
    if ((addr & (len - 1)) != 0) {
      //Log("misalgined load addr = " FMT_WORD ", pc = " FMT_WORD", instr = %x",
      //    addr, cpu.pc, decinfo.isa.instr.val);
      mtval->val = addr;
      if (cpu.amo) {
        cpu.amo = false;
        longjmp_raise_intr(EX_SAM);
      }
      longjmp_raise_intr(EX_LAM);
    }
  }
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
  if ((addr & (len - 1)) != 0) {
    //Log("misalgined store addr = " FMT_WORD ", pc = " FMT_WORD", instr = %x",
    //    addr, cpu.pc, decinfo.isa.instr.val);
    if (cpu.amo) cpu.amo = false;
    mtval->val = addr;
    longjmp_raise_intr(EX_SAM);
  }
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
*/

#define make_isa_vaddr_template(bits) \
uint_type(bits) concat(isa_vaddr_read, bits) (vaddr_t addr) { \
  if (!cpu.fetching) { \
    if ((addr & (bits / 8 - 1)) != 0) { \
      mtval->val = addr; \
      cpu.mem_exception = (cpu.amo ? EX_SAM : EX_LAM); \
      return 0; \
    } \
  } \
  paddr_t paddr; \
  if (!page_translate(addr, &paddr, false)) return 0; \
  return concat(paddr_read, bits)(paddr); \
} \
void concat(isa_vaddr_write, bits) (vaddr_t addr, uint_type(bits) data) { \
  if ((addr & (bits / 8 - 1)) != 0) { \
    mtval->val = addr; \
    cpu.mem_exception = EX_SAM; \
    return; \
  } \
  paddr_t paddr; \
  if (!page_translate(addr, &paddr, true)) return; \
  concat(paddr_write, bits)(paddr, data); \
}
 
make_isa_vaddr_template(8)
make_isa_vaddr_template(16)
make_isa_vaddr_template(32)
make_isa_vaddr_template(64)
