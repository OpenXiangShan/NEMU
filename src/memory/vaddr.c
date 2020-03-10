#include <isa.h>
#include <memory/paddr.h>
#include <memory/vaddr.h>

uint64_t vaddr_read_cross_page(vaddr_t addr, int type, int len) {
  uint64_t data = 0;
  int i;
  for (i = 0; i < len; i ++, addr ++) {
    paddr_t mmu_ret = isa_mmu_translate(addr, type, 1);
    int ret = mmu_ret & PAGE_MASK;
    if (ret != MEM_RET_OK) return 0;
    paddr_t paddr = (mmu_ret & ~PAGE_MASK) | (addr & PAGE_MASK);
    data |= (uint64_t)paddr_read8(paddr) << (i << 3);
  }
  return data;
}

void vaddr_write_cross_page(vaddr_t addr, uint64_t data, int len) {
  int i;
  for (i = 0; i < len; i ++, addr ++) {
    paddr_t mmu_ret = isa_mmu_translate(addr, MEM_TYPE_WRITE, 1);
    int ret = mmu_ret & PAGE_MASK;
    if (ret != MEM_RET_OK) return;
    paddr_t paddr = (mmu_ret & ~PAGE_MASK) | (addr & PAGE_MASK);
    paddr_write8(paddr, data & 0xff);
    data >>= 8;
  }
}

#define make_vaddr_read_template(bits, name, type) \
uint_type(bits) concat3(vaddr_, name, bits) (vaddr_t addr) { \
  int ret = isa_vaddr_check(addr, type, bits / 8); \
  if (ret == MEM_RET_OK) { \
    return concat(paddr_read, bits)(addr); \
  } else if (ret == MEM_RET_NEED_TRANSLATE) { \
    paddr_t pg_base = isa_mmu_translate(addr, type, bits / 8); \
    int ret = pg_base & PAGE_MASK; \
    if (ret == MEM_RET_OK) { \
      paddr_t paddr = pg_base | (addr & PAGE_MASK); \
      return concat(paddr_read, bits)(paddr); \
    } else if (bits != 8 && ret == MEM_RET_CROSS_PAGE) { \
      return vaddr_read_cross_page(addr, type, bits / 8); \
    } \
  } \
  return 0; \
}

#define make_vaddr_template(bits) \
  make_vaddr_read_template(bits, ifetch, MEM_TYPE_IFETCH) \
  make_vaddr_read_template(bits, read, MEM_TYPE_READ) \
void concat(vaddr_write, bits) (vaddr_t addr, uint_type(bits) data) { \
  int ret = isa_vaddr_check(addr, MEM_TYPE_READ, bits / 8); \
  if (ret == MEM_RET_OK) { \
    concat(paddr_write, bits)(addr, data); \
  } else if (ret == MEM_RET_NEED_TRANSLATE) { \
    paddr_t pg_base = isa_mmu_translate(addr, MEM_TYPE_WRITE, bits / 8); \
    int ret = pg_base & PAGE_MASK; \
    if (ret == MEM_RET_OK) { \
      paddr_t paddr = pg_base | (addr & PAGE_MASK); \
      concat(paddr_write, bits)(paddr, data); \
    } else if (bits != 8 && ret == MEM_RET_CROSS_PAGE) { \
      vaddr_write_cross_page(addr, data, bits / 8); \
    } \
  } \
}

make_vaddr_template(8)
make_vaddr_template(16)
make_vaddr_template(32)
#ifdef ISA64
make_vaddr_template(64)
#endif
