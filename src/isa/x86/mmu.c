#include <isa.h>
#include <memory/vaddr.h>
#include <memory/paddr.h>
#include "local-include/mmu.h"

typedef union {
  struct {
    uint32_t pf_off		:12;
    uint32_t pt_idx		:10;
    uint32_t pdir_idx	:10;
  };
  uint32_t addr;
} PageAddr;

static inline paddr_t ptw(vaddr_t vaddr, int type) {
  PageAddr *addr = (void *)&vaddr;
  paddr_t pdir_base = cpu.cr3.val & ~PAGE_MASK;

  PDE pde;
  pde.val	= paddr_read(pdir_base + addr->pdir_idx * 4, 4);
  if (!pde.present) {
    panic("pc = %x, vaddr = %x, pdir_base = %x, pde = %x", cpu.pc, vaddr, pdir_base, pde.val);
  }

  paddr_t pt_base = pde.val & ~PAGE_MASK;
  PTE pte;
  pte.val = paddr_read(pt_base + addr->pt_idx * 4, 4);
  if (!pte.present) {
    panic("pc = %x, vaddr = %x, pt_base = %x, pte = %x", cpu.pc, vaddr, pt_base, pte.val);
  }

  if (!pde.accessed) {
    pde.accessed = 1;
    paddr_write(pdir_base + addr->pdir_idx * 4,  pde.val, 4);
  }
  bool is_write = (type == MEM_TYPE_WRITE);
  if (!pte.accessed || (pte.dirty == 0 && is_write)) {
    pte.accessed = 1;
    pte.dirty |= is_write;
    paddr_write(pt_base + addr->pt_idx * 4, pte.val, 4);
  }

  return pte.val & ~PAGE_MASK;
}

paddr_t isa_mmu_translate(vaddr_t vaddr, int type, int len) {
  bool is_cross_page = ((vaddr & PAGE_MASK) + len) > PAGE_SIZE;
  if (is_cross_page) return MEM_RET_CROSS_PAGE;
  return ptw(vaddr, type) | MEM_RET_OK;
}
