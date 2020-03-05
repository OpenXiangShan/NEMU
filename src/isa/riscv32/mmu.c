#include <isa.h>
#include <memory/memory.h>

/* the 32bit Page Table Entry(second level page table) data structure */
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
    uint32_t ppn    :22;
  };
  uint32_t val;
} PTE;

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
  paddr_t pdir_base = cpu.satp.ppn << 12;

  PTE pde;
  pde.val	= paddr_read(pdir_base + addr->pdir_idx * 4, 4);
  if (!pde.valid) {
    panic("pc = %x, vaddr = %x, pdir_base = %x, pde = %x", cpu.pc, vaddr, pdir_base, pde.val);
  }

  paddr_t pt_base = pde.ppn << 12;
  PTE pte;
  pte.val = paddr_read(pt_base + addr->pt_idx * 4, 4);
  if (!pte.valid) {
    panic("pc = %x, vaddr = %x, pt_base = %x, pte = %x", cpu.pc, vaddr, pt_base, pte.val);
  }

  if (!pte.access || (pte.dirty == 0 && is_write)) {
    pte.access = 1;
    pte.dirty |= is_write;
    paddr_write(pt_base + addr->pt_idx * 4, pte.val, 4);
  }

  return pte.ppn << 12;
}

static inline paddr_t page_translate(vaddr_t addr, bool is_write) {
  return page_walk(addr, is_write) | (addr & PAGE_MASK);
}

#define make_isa_vaddr_template(bits) \
uint_type(bits) concat(isa_vaddr_read, bits) (vaddr_t addr) { \
  paddr_t paddr = (cpu.satp.mode ? page_translate(addr, false) : addr); \
  return concat(paddr_read, bits)(paddr); \
} \
void concat(isa_vaddr_write, bits) (vaddr_t addr, uint_type(bits) data) { \
  paddr_t paddr = (cpu.satp.mode ? page_translate(addr, true) : addr); \
  concat(paddr_write, bits)(paddr, data); \
}

make_isa_vaddr_template(8)
make_isa_vaddr_template(16)
make_isa_vaddr_template(32)
