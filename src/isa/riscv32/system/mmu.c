#include <isa.h>
#include <memory/paddr.h>
#include <memory/vaddr.h>

#ifndef __ICS_EXPORT
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

typedef struct {
  uint32_t vpn;
  uint32_t ppage;
} TLBEntry;

static TLBEntry TLB[4096] = {};
static bool TLBValid[4096] = {};

static inline int TLB_hash(uint32_t vpn) {
  return vpn % ARRLEN(TLB);
}

static paddr_t ptw(vaddr_t vaddr, int type) {
  uint32_t vpn = vaddr >> 12;
  int idx = TLB_hash(vpn);
  TLBEntry *e = &TLB[idx];
  //if (e->vpn == vpn && TLBValid[idx]) {
  //  if (cpu.pc == 0x40000120) Log("vaddr = 0x%x, ppage = 0x%x", vaddr, e->ppage);
  //  return e->ppage;
  //}

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

  // update TLB
  *e = (TLBEntry) { .vpn = vpn, .ppage = pte.ppn << 12 };
  TLBValid[idx] = true;

  //bool is_write = (type == MEM_TYPE_WRITE);
  //if (!pte.access || (pte.dirty == 0 && is_write)) {
  //  pte.access = 1;
  //  pte.dirty |= is_write;
  //  paddr_write(pt_base + addr->pt_idx * 4, 4, pte.val);
  //}

  return pte.ppn << 12;
}

paddr_t isa_mmu_translate(vaddr_t vaddr, int len, int type) {
  return ptw(vaddr, type) | MEM_RET_OK;
}
#else
paddr_t isa_mmu_translate(vaddr_t vaddr, int len, int type) {
  return MEM_RET_FAIL;
}
#endif
