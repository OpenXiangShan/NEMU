#ifndef __MEMORY_PADDR_H__
#define __MEMORY_PADDR_H__

#include <common.h>

#ifdef CONFIG_MODE_USER
#define CONFIG_MBASE 0
#define CONFIG_MSIZE 0
#define CONFIG_PC_RESET_OFFSET 0
#endif

#define RESET_VECTOR (CONFIG_MBASE + CONFIG_PC_RESET_OFFSET)

void init_mem();

/* convert the guest physical address in the guest program to host virtual address in NEMU */
uint8_t* guest_to_host(paddr_t paddr);
/* convert the host virtual address in NEMU to guest physical address in the guest program */
paddr_t host_to_guest(uint8_t *haddr);

static inline bool in_pmem(paddr_t addr) {
  paddr_t mbase_mask = CONFIG_MBASE - 1;
  paddr_t msize_mask = CONFIG_MSIZE - 1;
  bool mbase_align = (CONFIG_MBASE & mbase_mask) == 0;
  bool msize_align = (CONFIG_MSIZE & msize_mask) == 0;
  bool msize_inside_mbase = msize_mask <= mbase_mask;
  if (mbase_align && msize_align && msize_inside_mbase) {
    return (addr & ~msize_mask) == CONFIG_MBASE;
  } else {
    return (addr >= CONFIG_MBASE) && (addr < (paddr_t)CONFIG_MBASE + CONFIG_MSIZE);
  }
}

word_t paddr_read(paddr_t addr, int len);
void paddr_write(paddr_t addr, int len, word_t data);

#ifdef CONFIG_DIFFTEST_STORE_COMMIT

#define STORE_QUEUE_SIZE 48
typedef struct {
    uint64_t addr;
    uint64_t data;
    uint8_t  mask;
    uint8_t  valid;
} store_commit_t;
extern store_commit_t store_commit_queue[STORE_QUEUE_SIZE];

void store_commit_queue_push(uint64_t addr, uint64_t data, int len);
store_commit_t *store_commit_queue_pop();
int check_store_commit(uint64_t *addr, uint64_t *data, uint8_t *mask);
#endif

#ifdef CONFIG_MULTICORE_DIFF
extern uint8_t* golden_pmem;

static inline word_t golden_pmem_read(paddr_t addr, int len) {
  assert(golden_pmem != NULL);
  void *p = &golden_pmem[addr - 0x80000000];
  switch (len) {
    case 1: return *(uint8_t  *)p;
    case 2: return *(uint16_t *)p;
    case 4: return *(uint32_t *)p;
    case 8: return *(uint64_t *)p;
    default: assert(0);
  }
}
#endif

#endif
