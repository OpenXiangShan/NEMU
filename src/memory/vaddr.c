#include <isa.h>

#ifdef CONFIG_PERF_OPT
#define ENABLE_HOSTTLB 1
#endif

#include <memory/paddr.h>
#include <memory/vaddr.h>
#include <memory/host-tlb.h>

#ifndef __ICS_EXPORT
#ifndef ENABLE_HOSTTLB
static word_t vaddr_read_cross_page(vaddr_t addr, int len, int type) {
  word_t data = 0;
  int i;
  for (i = 0; i < len; i ++, addr ++) {
    paddr_t mmu_ret = isa_mmu_translate(addr, 1, type);
    int ret = mmu_ret & PAGE_MASK;
    if (ret != MEM_RET_OK) return 0;
    paddr_t paddr = (mmu_ret & ~PAGE_MASK) | (addr & PAGE_MASK);
#ifdef CONFIG_MULTICORE_DIFF
    word_t byte = (type == MEM_TYPE_IFETCH ? golden_pmem_read : paddr_read)(paddr, 1);
#else
    word_t byte = (type == MEM_TYPE_IFETCH ? paddr_read : paddr_read)(paddr, 1);
#endif
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
static word_t vaddr_mmu_read(struct Decode *s, vaddr_t addr, int len, int type) {
#ifdef XIANGSHAN_DEBUG
  vaddr_t vaddr = addr;
#endif
  paddr_t pg_base = isa_mmu_translate(addr, len, type);
  int ret = pg_base & PAGE_MASK;
  if (ret == MEM_RET_OK) {
    addr = pg_base | (addr & PAGE_MASK);
#ifdef CONFIG_MULTICORE_DIFF
    word_t rdata = (type == MEM_TYPE_IFETCH ? golden_pmem_read : paddr_read)(addr, len);
#else
    word_t rdata = paddr_read(addr, len);
#endif
#ifdef XIANGSHAN_DEBUG
    printf("[NEMU] mmu_read: vaddr 0x%lx, paddr 0x%lx, rdata 0x%lx\n",
      vaddr, addr, rdata);
#endif
    return rdata;
  } else if (len != 1 && ret == MEM_RET_CROSS_PAGE) {
    return vaddr_read_cross_page(addr, len, type);
  }
  return 0;
}

__attribute__((noinline))
static void vaddr_mmu_write(struct Decode *s, vaddr_t addr, int len, word_t data) {
#ifdef XIANGSHAN_DEBUG
  vaddr_t vaddr = addr;
#endif
  paddr_t pg_base = isa_mmu_translate(addr, len, MEM_TYPE_WRITE);
  int ret = pg_base & PAGE_MASK;
  if (ret == MEM_RET_OK) {
    addr = pg_base | (addr & PAGE_MASK);
#ifdef XIANGSHAN_DEBUG
    printf("[NEMU] mmu_write: vaddr 0x%lx, paddr 0x%lx, len %d, data 0x%lx\n",
      vaddr, addr, len, data);
#endif
    paddr_write(addr, len, data);
  } else if (len != 1 && ret == MEM_RET_CROSS_PAGE) {
    vaddr_write_cross_page(addr, len, data);
  }
}
#endif
#endif

static inline word_t vaddr_read_internal(void *s, vaddr_t addr, int len, int type, int mmu_mode) {
#ifdef CONFIG_SHARE
  void isa_misalign_data_addr_check(vaddr_t vaddr, int len, int type);
  if (type != MEM_TYPE_IFETCH) {
    isa_misalign_data_addr_check(addr, len, type);
  }
#endif
  if (unlikely(mmu_mode == MMU_DYNAMIC)) mmu_mode = isa_mmu_check(addr, len, type);
  if (mmu_mode == MMU_DIRECT) return paddr_read(addr, len);
#ifndef __ICS_EXPORT
  return MUXDEF(ENABLE_HOSTTLB, hosttlb_read, vaddr_mmu_read) ((struct Decode *)s, addr, len, type);
#endif
  return 0;
}

word_t vaddr_ifetch(vaddr_t addr, int len) {
  return vaddr_read_internal(NULL, addr, len, MEM_TYPE_IFETCH, MMU_DYNAMIC);
}

word_t vaddr_read(struct Decode *s, vaddr_t addr, int len, int mmu_mode) {
  return vaddr_read_internal(s, addr, len, MEM_TYPE_READ, mmu_mode);
}

void vaddr_write(struct Decode *s, vaddr_t addr, int len, word_t data, int mmu_mode) {
#ifdef CONFIG_SHARE
  void isa_misalign_data_addr_check(vaddr_t vaddr, int len, int type);
  isa_misalign_data_addr_check(addr, len, MEM_TYPE_WRITE);
#endif
  if (unlikely(mmu_mode == MMU_DYNAMIC)) mmu_mode = isa_mmu_check(addr, len, MEM_TYPE_WRITE);
  if (mmu_mode == MMU_DIRECT) { paddr_write(addr, len, data); return; }
#ifndef __ICS_EXPORT
  MUXDEF(ENABLE_HOSTTLB, hosttlb_write, vaddr_mmu_write) (s, addr, len, data);
#endif
}

word_t vaddr_read_safe(vaddr_t addr, int len) {
  // FIXME: when reading fails, return an error instead of raising exceptions
  return vaddr_read_internal(NULL, addr, len, MEM_TYPE_READ, MMU_DYNAMIC);
}
