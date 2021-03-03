#include <isa.h>
#include <memory/paddr.h>
#include <memory/vaddr.h>

static word_t vaddr_read_cross_page(vaddr_t addr, int type, int len) {
  word_t data = 0;
  int i;
  for (i = 0; i < len; i ++, addr ++) {
    paddr_t mmu_ret = isa_mmu_translate(addr, type, 1);
    int ret = mmu_ret & PAGE_MASK;
    if (ret != MEM_RET_OK) return 0;
    paddr_t paddr = (mmu_ret & ~PAGE_MASK) | (addr & PAGE_MASK);
    word_t byte = (type == MEM_TYPE_IFETCH ? paddr_read : paddr_read)(paddr, 1);
    data |= byte << (i << 3);
  }
  return data;
}

static void vaddr_write_cross_page(vaddr_t addr, word_t data, int len) {
  int i;
  for (i = 0; i < len; i ++, addr ++) {
    paddr_t mmu_ret = isa_mmu_translate(addr, MEM_TYPE_WRITE, 1);
    int ret = mmu_ret & PAGE_MASK;
    if (ret != MEM_RET_OK) return;
    paddr_t paddr = (mmu_ret & ~PAGE_MASK) | (addr & PAGE_MASK);
    paddr_write(paddr, data & 0xff, 1);
    data >>= 8;
  }
}

word_t vaddr_mmu_read(vaddr_t addr, int len, int type) {
#ifdef XIANGSHAN_DEBUG
  vaddr_t vaddr = addr;
#endif
  paddr_t pg_base = isa_mmu_translate(addr, type, len);
  int ret = pg_base & PAGE_MASK;
  if (ret == MEM_RET_OK) {
    addr = pg_base | (addr & PAGE_MASK);
    word_t rdata = paddr_read(addr, len);
#ifdef XIANGSHAN_DEBUG
    printf("[NEMU] mmu_read: vaddr 0x%lx, paddr 0x%lx, rdata 0x%lx\n",
      vaddr, addr, rdata);
#endif
    return rdata;
  } else if (len != 1 && ret == MEM_RET_CROSS_PAGE) {
    return vaddr_read_cross_page(addr, type, len);
  }
  return 0;
}

void vaddr_mmu_write(vaddr_t addr, word_t data, int len) {
#ifdef XIANGSHAN_DEBUG
  vaddr_t vaddr = addr;
#endif
  paddr_t pg_base = isa_mmu_translate(addr, MEM_TYPE_WRITE, len);
  int ret = pg_base & PAGE_MASK;
  if (ret == MEM_RET_OK) {
    addr = pg_base | (addr & PAGE_MASK);
#ifdef XIANGSHAN_DEBUG
    printf("[NEMU] mmu_write: vaddr 0x%lx, paddr 0x%lx, len %d, data 0x%lx\n",
      vaddr, addr, len, data);
#endif
    paddr_write(addr, data, len);
  } else if (len != 1 && ret == MEM_RET_CROSS_PAGE) {
    vaddr_write_cross_page(addr, data, len);
  }
}
