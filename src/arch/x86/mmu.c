#include "nemu.h"
#include "arch/mmu.h"

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
    panic("eip = %x, vaddr = %x, pdir_base = %x, pde = %x", cpu.eip, vaddr, pdir_base, pde.val);
  }

  paddr_t pt_base = pde.val & ~PAGE_MASK;
  PTE pte;
  pte.val = paddr_read(pt_base + addr->pt_idx * 4, 4);
  if (!pte.present) {
    panic("eip = %x, vaddr = %x, pt_base = %x, pte = %x", cpu.eip, vaddr, pt_base, pte.val);
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

uint32_t arch_vaddr_read(vaddr_t addr, int len) {
  uint32_t data;
  if (cpu.cr0.paging) {
    paddr_t paddr = page_translate(addr, false);
    uint32_t remain_byte = PAGE_SIZE - (addr & PAGE_MASK);
    if (remain_byte < len) {
      /* data cross the page boundary */
      data = paddr_read(paddr, remain_byte);

      paddr = page_translate(addr + remain_byte, false);
      data |= paddr_read(paddr, len - remain_byte) << (remain_byte << 3);
    }
    else {
      data = paddr_read(paddr, len);
    }
  }
  else {
    data = paddr_read(addr, len);
  }

  return data;
}

void arch_vaddr_write(vaddr_t addr, uint32_t data, int len) {
  if (cpu.cr0.paging) {
    paddr_t paddr = page_translate(addr, true);
    uint32_t remain_byte = PAGE_SIZE - (addr & PAGE_MASK);
    if (remain_byte < len) {
      /* data cross the page boundary */
      uint32_t cut = PAGE_SIZE - (addr & PAGE_MASK);
      assert(cut < 4);
      paddr_write(paddr, data, cut);

      paddr = page_translate(addr + cut, true);
      paddr_write(paddr, data >> (cut << 3), len - cut);
    }
    else {
      paddr_write(paddr, data, len);
    }
  }
  else {
    paddr_write(addr, data, len);
  }
}
