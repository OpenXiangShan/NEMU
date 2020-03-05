#include <isa.h>
#include <memory/memory.h>
#include "local-include/mmu.h"

typedef union {
  struct {
    uint32_t pf_off		:12;
    uint32_t pt_idx		:10;
    uint32_t pdir_idx	:10;
  };
  uint32_t addr;
} PageAddr;

static paddr_t page_walk(vaddr_t vaddr, bool is_write) {
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
  if (!pte.accessed || (pte.dirty == 0 && is_write)) {
    pte.accessed = 1;
    pte.dirty |= is_write;
    paddr_write(pt_base + addr->pt_idx * 4, pte.val, 4);
  }

  return pte.val & ~PAGE_MASK;
}

static inline paddr_t page_translate(vaddr_t addr, bool is_write) {
  return page_walk(addr, is_write) | (addr & PAGE_MASK);
}

#define make_isa_vaddr_template(bits) \
uint_type(bits) concat(isa_vaddr_read, bits) (vaddr_t addr) { \
  paddr_t paddr = addr; \
  if (cpu.cr0.paging) { \
    paddr = page_translate(addr, false); \
    bool is_cross_page = ((addr & PAGE_MASK) + bits / 8) > PAGE_SIZE; \
    if (is_cross_page) { \
      uint_type(bits) data = 0; \
      int i = 0; \
      while (true) { \
        data |= paddr_read8(paddr) << (i << 3); \
        i ++; \
        if (i == bits / 8) break; \
        addr ++; \
        paddr = page_translate(addr, false); \
      } \
      return data; \
    } \
  } \
  return concat(paddr_read, bits)(paddr); \
} \
void concat(isa_vaddr_write, bits) (vaddr_t addr, uint_type(bits) data) { \
  paddr_t paddr = addr; \
  if (cpu.cr0.paging) { \
    paddr = page_translate(addr, false); \
    bool is_cross_page = ((addr & PAGE_MASK) + bits / 8) > PAGE_SIZE; \
    if (is_cross_page) { \
      int i = 0; \
      while (true) { \
        paddr_write8(paddr, data & 0xff); \
        i ++; \
        if (i == bits / 8) break; \
        addr ++; \
        paddr = page_translate(addr, false); \
      } \
      return; \
    } \
  } \
  concat(paddr_write, bits)(paddr, data); \
}

make_isa_vaddr_template(8)
make_isa_vaddr_template(16)
make_isa_vaddr_template(32)
