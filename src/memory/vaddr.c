#include <isa.h>
#include <memory/paddr.h>
#include <memory/vaddr.h>

#ifdef CONFIG_MODE_USER
#define vaddr2uint8(addr)  (uint8_t  *)(void *)(uintptr_t)(addr)
#define vaddr2uint16(addr) (uint16_t *)(void *)(uintptr_t)(addr)
#define vaddr2uint32(addr) (uint32_t *)(void *)(uintptr_t)(addr)
#define vaddr2uint64(addr) (uint64_t *)(void *)(uintptr_t)(addr)

static inline word_t vaddr_read(vaddr_t addr, int len) {
  switch (len) {
    case 1: return *vaddr2uint8 (addr);
    case 2: return *vaddr2uint16(addr);
    case 4: return *vaddr2uint32(addr);
    IFDEF(CONFIG_ISA64, case 8: return *vaddr2uint64(addr));
    default: assert(0);
  }
}

static inline void vaddr_write(vaddr_t addr, int len, word_t data) {
  switch (len) {
    case 1: *vaddr2uint8 (addr) = data; break;
    case 2: *vaddr2uint16(addr) = data; break;
    case 4: *vaddr2uint32(addr) = data; break;
    IFDEF(CONFIG_ISA64, case 8: *vaddr2uint64(addr) = data; break);
    default: assert(0);
  }
}

static inline word_t vaddr_ifetch(vaddr_t addr, int len) {
  return vaddr_read(addr, len);
}

#else

#ifndef __ICS_EXPORT
static word_t vaddr_read_cross_page(vaddr_t addr, int len, int type) {
  word_t data = 0;
  int i;
  for (i = 0; i < len; i ++, addr ++) {
    paddr_t mmu_ret = isa_mmu_translate(addr, 1, type);
    int ret = mmu_ret & PAGE_MASK;
    if (ret != MEM_RET_OK) return 0;
    paddr_t paddr = (mmu_ret & ~PAGE_MASK) | (addr & PAGE_MASK);
    word_t byte = (type == MEM_TYPE_IFETCH ? paddr_read : paddr_read)(paddr, 1);
    data |= byte << (i << 3);
  }
  return data;
}

static void vaddr_write_cross_page(vaddr_t addr, int len, word_t data) {
  int i;
  for (i = 0; i < len; i ++, addr ++) {
    paddr_t mmu_ret = isa_mmu_translate(addr, 1, MEM_TYPE_WRITE);
    int ret = mmu_ret & PAGE_MASK;
    if (ret != MEM_RET_OK) return;
    paddr_t paddr = (mmu_ret & ~PAGE_MASK) | (addr & PAGE_MASK);
    paddr_write(paddr, 1, data & 0xff);
    data >>= 8;
  }
}

__attribute__((noinline))
static word_t vaddr_mmu_read(vaddr_t addr, int len, int type) {
  paddr_t pg_base = isa_mmu_translate(addr, len, type);
  int ret = pg_base & PAGE_MASK;
  if (ret == MEM_RET_OK) {
    addr = pg_base | (addr & PAGE_MASK);
    return paddr_read(addr, len);
  } else if (len != 1 && ret == MEM_RET_CROSS_PAGE) {
    return vaddr_read_cross_page(addr, len, type);
  }
  return 0;
}

__attribute__((noinline))
static void vaddr_mmu_write(vaddr_t addr, int len, word_t data) {
  paddr_t pg_base = isa_mmu_translate(addr, len, MEM_TYPE_WRITE);
  int ret = pg_base & PAGE_MASK;
  if (ret == MEM_RET_OK) {
    addr = pg_base | (addr & PAGE_MASK);
    paddr_write(addr, len, data);
  } else if (len != 1 && ret == MEM_RET_CROSS_PAGE) {
    vaddr_write_cross_page(addr, len, data);
  }
}
#endif

static word_t vaddr_read_internal_with_mmu_state(vaddr_t addr, int len, int type, int mmu_state) {
  if (mmu_state == MMU_DYNAMIC) mmu_state = isa_mmu_check(addr, len, type);
  if (mmu_state == MMU_DIRECT) return paddr_read(addr, len);
#ifndef __ICS_EXPORT
  else if (mmu_state == MMU_TRANSLATE) return vaddr_mmu_read(addr, len, type);
#endif
  return 0;
}

word_t vaddr_read_with_mmu_state(void *s, vaddr_t addr, int len, int mmu_state) {
  return vaddr_read_internal_with_mmu_state(addr, len, MEM_TYPE_READ, mmu_state);
}

void vaddr_write_with_mmu_state(void *s, vaddr_t addr, int len, word_t data, int mmu_state) {
  if (mmu_state == MMU_DYNAMIC) mmu_state = isa_mmu_check(addr, len, type);
  if (mmu_state == MMU_DIRECT) paddr_write(addr, len, data);
#ifndef __ICS_EXPORT
  else if (mmu_state == MMU_TRANSLATE) vaddr_mmu_write(addr, len, data);
#endif
}

word_t vaddr_ifetch(vaddr_t addr, int len) {
  return vaddr_read_internal_with_mmu_state(addr, len, MEM_TYPE_IFETCH, isa_mmu_state());
}

word_t vaddr_read(vaddr_t addr, int len) {
  return vaddr_read_internal_with_mmu_state(addr, len, MEM_TYPE_READ, isa_mmu_state());
}

void vaddr_write(vaddr_t addr, int len, word_t data) {
  vaddr_write_with_mmu_state(NULL, addr, len, data, isa_mmu_state());
}

#endif
