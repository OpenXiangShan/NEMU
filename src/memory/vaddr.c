#include <isa.h>
#include <memory/paddr.h>
#include <memory/vaddr.h>

extern word_t read_goldenmem(paddr_t addr, uint64_t len);

word_t vaddr_read_cross_page(vaddr_t addr, int type, int len) {
  word_t data = 0;
  int i;
  for (i = 0; i < len; i ++, addr ++) {
    paddr_t mmu_ret = isa_mmu_translate(addr, type, 1);
    int ret = mmu_ret & PAGE_MASK;
    if (ret != MEM_RET_OK) return 0;
    paddr_t paddr = (mmu_ret & ~PAGE_MASK) | (addr & PAGE_MASK);
#if _SHARE
    word_t byte = (type == MEM_TYPE_IFETCH ? paddr_read : paddr_read)(paddr, 1);
#else    
    word_t byte;
    if (type == MEM_TYPE_IFETCH) {
      byte = read_goldenmem(paddr, 1);
    } else {
      byte = paddr_read(paddr, 1);
    }
#endif
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
  paddr_t pg_base = isa_mmu_translate(addr, type, len);
  int ret = pg_base & PAGE_MASK;
  if (ret == MEM_RET_OK) {
    addr = pg_base | (addr & PAGE_MASK);
#if _SHARE
    if (type == MEM_TYPE_IFETCH) {
      extern word_t read_goldenmem(paddr_t addr, uint64_t len);
      return read_goldenmem(addr, len);
    } else {
      return paddr_read(addr, len);
    }
#else
    return paddr_read(addr, len);
#endif
  } else if (len != 1 && ret == MEM_RET_CROSS_PAGE) {
    return vaddr_read_cross_page(addr, type, len);
  }
  return 0;
}

void vaddr_mmu_write(vaddr_t addr, word_t data, int len) {
  paddr_t pg_base = isa_mmu_translate(addr, MEM_TYPE_WRITE, len);
  int ret = pg_base & PAGE_MASK;
  if (ret == MEM_RET_OK) {
    addr = pg_base | (addr & PAGE_MASK);
    paddr_write(addr, data, len);
  } else if (len != 1 && ret == MEM_RET_CROSS_PAGE) {
    vaddr_write_cross_page(addr, data, len);
  }
}
