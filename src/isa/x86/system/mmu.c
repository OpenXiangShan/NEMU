#include <isa.h>
#include <memory/paddr.h>
#include <memory/vaddr.h>
#ifndef __ICS_EXPORT
#include "../local-include/reg.h"
#include <cpu/difftest.h>

typedef union PageTableEntry {
  struct {
    uint32_t p    : 1;
    uint32_t w    : 1;
    uint32_t u    : 1;
    uint32_t pwt  : 1;
    uint32_t pcd  : 1;
    uint32_t a    : 1;
    uint32_t d    : 1;
    uint32_t pat  : 1;
    uint32_t g    : 1;
    uint32_t pad  : 3;
    uint32_t ppn  :20;
  };
  uint32_t val;
} PTE;

#define PGSHFT 12
#define PGMASK ((1u << PGSHFT) - 1)
#define PGBASE(pn) (pn << PGSHFT)

#define PTW_LEVEL 2
#define PTE_SIZE 4
#define VPNMASK 0x3ff

static inline word_t VPNiSHFT(int i) {
  return PGSHFT + 10 * i;
}

static inline word_t VPNi(vaddr_t va, int i) {
  return (va >> VPNiSHFT(i)) & VPNMASK;
}

#ifdef CONFIG_PA
static inline bool check_permission(PTE *pte, bool ok, vaddr_t vaddr, int type) {
  Assert(pte->p, "vaddr = %x, cpu.pc = %x", vaddr, cpu.pc);
  return true;
}
#else
static inline bool check_permission(PTE *pte, bool ok, vaddr_t vaddr, int type) {
  int is_user = cpu.sreg[CSR_CS].rpl == MODE_R3;
  int is_write = (type == MEM_TYPE_WRITE) || (type == MEM_TYPE_READ && cpu.lock);
  ok = ok && pte->p;
  ok = ok && !(is_user && !pte->u);
  ok = ok && !(is_write && !pte->w); // assume that CR0.WP is always enabled
  if (!ok && cpu.mem_exception == 0) {
    cpu.cr2 = vaddr;
    cpu.mem_exception = 14;
    cpu.lock = 0;
    cpu.error_code = pte->p | (is_write << 1) | (is_user << 2);
  }
  return ok;
}
#endif

static inline paddr_t ptw(vaddr_t vaddr, int type) {
  word_t pg_base = PGBASE(cpu.cr3.ppn);
  word_t p_pte[PTW_LEVEL]; // pte pointer
  PTE pte[PTW_LEVEL];
  int level;

  for (level = PTW_LEVEL - 1; level >= 0; level --) {
    p_pte[level] = pg_base + VPNi(vaddr, level) * PTE_SIZE;
    pte[level].val = paddr_read(p_pte[level], PTE_SIZE);
    pg_base = PGBASE(pte[level].ppn);
    if (!pte[level].p) goto bad;
  }

  level ++;
  assert(level == 0);
  if (!check_permission(&pte[0], true, vaddr, type)) return MEM_RET_FAIL;

#if !defined(CONFIG_PA) || defined(CONFIG_DIFFTEST)
  if (!pte[1].a) {
    pte[1].a = 1;
    paddr_write(p_pte[1], PTE_SIZE, pte[1].val);
    IFDEF(CONFIG_DIFFTEST,
        ref_difftest_memcpy(p_pte[1], &pte[1].val, PTE_SIZE, DIFFTEST_TO_REF));
  }
  bool is_write = (type == MEM_TYPE_WRITE);
  if (!pte[0].a || (!pte[0].d && is_write)) {
    pte[0].a = 1;
    pte[0].d |= is_write;
    paddr_write(p_pte[0], PTE_SIZE, pte[0].val);
    IFDEF(CONFIG_DIFFTEST,
        ref_difftest_memcpy(p_pte[0], &pte[0].val, PTE_SIZE, DIFFTEST_TO_REF));
  }
#endif

  return pg_base | MEM_RET_OK;

bad:
  check_permission(&pte[level], false, vaddr, type);
  return MEM_RET_FAIL;
}

paddr_t isa_mmu_translate(vaddr_t vaddr, int len, int type) {
  bool is_cross_page = ((vaddr & PAGE_MASK) + len) > PAGE_SIZE;
  if (is_cross_page) return MEM_RET_CROSS_PAGE;
  return ptw(vaddr, type);
}
#else

paddr_t isa_mmu_translate(vaddr_t vaddr, int len, int type) {
  return MEM_RET_FAIL;
}
#endif
