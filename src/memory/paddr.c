#include <memory/host.h>
#include <device/mmio.h>
#include <stdlib.h>
#include <time.h>

static uint8_t pmem[CONFIG_MSIZE] PG_ALIGN = {};

#define PMEM_MASK (CONFIG_MSIZE - 1)
#define HOST_PMEM_OFFSET (void *)(pmem - CONFIG_MBASE)

word_t mmio_read(paddr_t addr, int len);
void mmio_write(paddr_t addr, int len, word_t data);

void* guest_to_host(paddr_t paddr) { return paddr + HOST_PMEM_OFFSET; }
paddr_t host_to_guest(void *haddr) { return haddr - HOST_PMEM_OFFSET; }

static inline bool in_pmem(paddr_t addr) {
  return (addr & ~PMEM_MASK) == CONFIG_MBASE;
}

static inline word_t pmem_read(paddr_t addr, int len) {
  return host_read(guest_to_host(addr), len);
}

static inline void pmem_write(paddr_t addr, int len, word_t data) {
  host_write(guest_to_host(addr), len, data);
}

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

/* Memory accessing interfaces */

word_t paddr_read(paddr_t addr, int len) {
  if (likely(in_pmem(addr))) return pmem_read(addr, len);
  else return mmio_read(addr, len);
}

void paddr_write(paddr_t addr, int len, word_t data) {
  if (likely(in_pmem(addr))) pmem_write(addr, len, data);
  else mmio_write(addr, len, data);
}
