#include <isa.h>
#include <memory/paddr.h>
#include <memory/vaddr.h>
#include <stdlib.h>
#include <time.h>

word_t mmio_read(paddr_t addr, int len);
void mmio_write(paddr_t addr, int len, word_t data);

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

static inline void pmem_write(uint32_t idx, int len, word_t data) {
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

word_t paddr_read(paddr_t addr, int len) {
  uint32_t idx = addr - CONFIG_MBASE;
  if (likely(in_pmem(idx))) return pmem_read(idx, len);
  else return mmio_read(addr, len);
}

void paddr_write(paddr_t addr, int len, word_t data) {
  uint32_t idx = addr - CONFIG_MBASE;
  if (likely(in_pmem(idx))) pmem_write(idx, len, data);
  else mmio_write(addr, len, data);
}
