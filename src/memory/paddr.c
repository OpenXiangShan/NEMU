#include <memory/host.h>
#include <memory/paddr.h>
#include <device/mmio.h>
#include <stdlib.h>
#include <time.h>

#ifdef CONFIG_USE_MMAP
#include <sys/mman.h>
static const uint8_t *pmem = (void *)0x100000000ul;
#else
static uint8_t pmem[CONFIG_MSIZE] PG_ALIGN = {};
#endif
#define HOST_PMEM_OFFSET (void *)(pmem - CONFIG_MBASE)

void* guest_to_host(paddr_t paddr) { return paddr + HOST_PMEM_OFFSET; }
paddr_t host_to_guest(void *haddr) { return haddr - HOST_PMEM_OFFSET; }

static inline word_t pmem_read(paddr_t addr, int len) {
  return host_read(guest_to_host(addr), len);
}

static inline void pmem_write(paddr_t addr, int len, word_t data) {
  host_write(guest_to_host(addr), len, data);
}

void init_mem() {
#ifdef CONFIG_USE_MMAP
  void *ret = mmap((void *)pmem, CONFIG_MSIZE, PROT_READ | PROT_WRITE,
      MAP_ANONYMOUS | MAP_PRIVATE | MAP_FIXED, -1, 0);
  if (ret != pmem) {
    perror("mmap");
    assert(0);
  }
#endif
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
