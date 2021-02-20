#include <isa.h>
#include <memory/paddr.h>
#include <memory/vaddr.h>
#include <stdlib.h>
#include <time.h>

word_t mmio_read(paddr_t addr, int len);
void mmio_write(paddr_t addr, word_t data, int len);

static uint8_t pmem[CONFIG_MSIZE] PG_ALIGN = {};

void* guest_to_host(paddr_t addr) { return &pmem[addr - CONFIG_MBASE]; }
paddr_t host_to_guest(void *addr) { return CONFIG_MBASE + (addr - (void *)pmem); }

void init_mem() {
#ifdef CONFIG_MEM_RANDOM
  srand(time(0));
  uint32_t *p = (uint32_t *)pmem;
  int i;
  for (i = 0; i < CONFIG_MSIZE / sizeof(p[0]); i ++) {
    p[i] = rand();
  }
#endif
}

static inline bool in_pmem(uint32_t idx) {
  return (idx < CONFIG_MSIZE);
}

static inline word_t pmem_read(uint32_t idx, int len) {
  void *p = &pmem[idx];
  switch (len) {
    case 1: return *(uint8_t  *)p;
    case 2: return *(uint16_t *)p;
    case 4: return *(uint32_t *)p;
    IFDEF(CONFIG_ISA64, case 8: return *(uint64_t *)p);
    default: assert(0);
  }
}

static inline void pmem_write(uint32_t idx, word_t data, int len) {
  void *p = &pmem[idx];
  switch (len) {
    case 1: *(uint8_t  *)p = data; return;
    case 2: *(uint16_t *)p = data; return;
    case 4: *(uint32_t *)p = data; return;
    IFDEF(CONFIG_ISA64, case 8: *(uint64_t *)p = data; return);
    default: assert(0);
  }
}

/* Memory accessing interfaces */

inline word_t paddr_read(paddr_t addr, int len) {
  uint32_t idx = addr - CONFIG_MBASE;
  if (likely(in_pmem(idx))) return pmem_read(idx, len);
  else return mmio_read(addr, len);
}

inline void paddr_write(paddr_t addr, word_t data, int len) {
  uint32_t idx = addr - CONFIG_MBASE;
  if (likely(in_pmem(idx))) pmem_write(idx, data, len);
  else mmio_write(addr, data, len);
}

word_t vaddr_mmu_read(vaddr_t addr, int len, int type);
void vaddr_mmu_write(vaddr_t addr, word_t data, int len);

#ifdef __ICS_EXPORT

#define def_vaddr_template(bytes) \
word_t concat(vaddr_ifetch, bytes) (vaddr_t addr) { \
  int ret = isa_vaddr_check(addr, MEM_TYPE_IFETCH, bytes); \
  if (ret == MEM_RET_OK) return paddr_read(addr, bytes); \
  return 0; \
} \
word_t concat(vaddr_read, bytes) (vaddr_t addr) { \
  int ret = isa_vaddr_check(addr, MEM_TYPE_READ, bytes); \
  if (ret == MEM_RET_OK) return paddr_read(addr, bytes); \
  return 0; \
} \
void concat(vaddr_write, bytes) (vaddr_t addr, word_t data) { \
  int ret = isa_vaddr_check(addr, MEM_TYPE_WRITE, bytes); \
  if (ret == MEM_RET_OK) paddr_write(addr, data, bytes); \
}

#else

#define def_vaddr_template(bytes) \
word_t concat(vaddr_ifetch, bytes) (vaddr_t addr) { \
  int ret = isa_vaddr_check(addr, MEM_TYPE_IFETCH, bytes); \
  if (ret == MEM_RET_OK) return paddr_read(addr, bytes); \
  else if (ret == MEM_RET_NEED_TRANSLATE) return vaddr_mmu_read(addr, bytes, MEM_TYPE_IFETCH); \
  return 0; \
} \
word_t concat(vaddr_read, bytes) (vaddr_t addr) { \
  int ret = isa_vaddr_check(addr, MEM_TYPE_READ, bytes); \
  if (ret == MEM_RET_OK) return paddr_read(addr, bytes); \
  else if (ret == MEM_RET_NEED_TRANSLATE) return vaddr_mmu_read(addr, bytes, MEM_TYPE_READ); \
  return 0; \
} \
void concat(vaddr_write, bytes) (vaddr_t addr, word_t data) { \
  int ret = isa_vaddr_check(addr, MEM_TYPE_WRITE, bytes); \
  if (ret == MEM_RET_OK) paddr_write(addr, data, bytes); \
  else if (ret == MEM_RET_NEED_TRANSLATE) vaddr_mmu_write(addr, data, bytes); \
}

#endif

def_vaddr_template(1)
def_vaddr_template(2)
def_vaddr_template(4)
IFDEF(CONFIG_ISA64, def_vaddr_template(8))
