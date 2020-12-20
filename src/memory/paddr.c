#include <isa.h>
#include <memory/paddr.h>
#include <memory/vaddr.h>
#include <device/map.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <time.h>

static uint8_t *pmem;//[PMEM_SIZE] PG_ALIGN = {};
#ifdef ENABLE_DISAMBIGUATE
static bool    *pmem_dirty;//[PMEM_SIZE] PG_ALIGN = {0};
#endif

void* guest_to_host(paddr_t addr) { return &pmem[addr]; }
paddr_t host_to_guest(void *addr) { return (void *)pmem - addr; }

IOMap* fetch_mmio_map(paddr_t addr);

void init_mem() {
  pmem = (uint8_t *)mmap(NULL, PMEM_SIZE, PROT_READ | PROT_WRITE, MAP_ANON | MAP_PRIVATE, -1, 0);
  if (pmem == (uint8_t *)MAP_FAILED) {
    printf("ERROR allocating physical memory. \n");
  }

#ifdef ENABLE_DISAMBIGUATE
  pmem_dirty = (bool *)mmap(NULL, PMEM_SIZE * sizeof(bool), PROT_READ | PROT_WRITE, MAP_ANON | MAP_PRIVATE, -1, 0);
  if (pmem_dirty == (bool *)MAP_FAILED) {
    printf("ERROR allcoating pmem_dirty bitmap. \n");
  }
#endif

#ifdef DIFFTEST_STORE_COMMIT
  for (int i = 0; i < STORE_QUEUE_SIZE; i++) {
    store_commit_queue[i].valid = 0;
  }
#endif

#if !defined(DIFF_TEST) && !_SHARE
  srand(time(0));
  uint32_t *p = (uint32_t *)pmem;
  uint64_t i;
  for (i = 0; i < PMEM_SIZE / sizeof(p[0]); i ++) {
    p[i] = rand();
  }
#endif
}

static inline bool in_pmem(paddr_t addr) {
  return (PMEM_BASE <= addr) && (addr <= PMEM_BASE + PMEM_SIZE - 1);
}

static inline word_t pmem_read(paddr_t addr, int len) {
  void *p = &pmem[addr - PMEM_BASE];
  switch (len) {
    case 1: return *(uint8_t  *)p;
    case 2: return *(uint16_t *)p;
    case 4: return *(uint32_t *)p;
#ifdef ISA64
    case 8: return *(uint64_t *)p;
#endif
    default: assert(0);
  }
}

static inline void pmem_write(paddr_t addr, word_t data, int len) {
#ifdef DIFFTEST_STORE_COMMIT
  store_commit_queue_push(addr, data, len);
#endif

  // write to pmem, mark pmem addr as dirty
  void *p = &pmem[addr - PMEM_BASE];
  switch (len) {
    case 1: 
      *(uint8_t  *)p = data;
#ifdef ENABLE_DISAMBIGUATE
      pmem_dirty[addr - PMEM_BASE] = true;
#endif
      return;
    case 2: 
      *(uint16_t *)p = data;
#ifdef ENABLE_DISAMBIGUATE
      for(int i = 0; i < 2; i++)
        pmem_dirty[addr - PMEM_BASE + i] = true;
#endif
      return;
    case 4: 
      *(uint32_t *)p = data;
#ifdef ENABLE_DISAMBIGUATE
      for(int i = 0; i < 4; i++)
        pmem_dirty[addr - PMEM_BASE + i] = true;
#endif
      return;
#ifdef ISA64
    case 8: 
      *(uint64_t *)p = data;
#ifdef ENABLE_DISAMBIGUATE
      for(int i = 0; i < 8; i++)
        pmem_dirty[addr - PMEM_BASE + i] = true;
#endif
      return;
#endif
    default: assert(0);
  }
}

void rtl_sfence() {
#ifdef ENABLE_DISAMBIGUATE
  memset(pmem_dirty, 0, sizeof(sizeof(bool) * PMEM_SIZE));
#endif
}

/* Memory accessing interfaces */

inline word_t paddr_read(paddr_t addr, int len) {
  if (in_pmem(addr)) return pmem_read(addr, len);
  else return map_read(addr, len, fetch_mmio_map(addr));
}

inline void paddr_write(paddr_t addr, word_t data, int len) {
  if (in_pmem(addr)) pmem_write(addr, data, len);
  else map_write(addr, data, len, fetch_mmio_map(addr));
}

#ifdef ENABLE_DISAMBIGUATE
bool is_sfence_safe(paddr_t addr, int len) {
  if (in_pmem(addr)){
    bool dirty = false;
    switch (len) {
      case 1: return !pmem_dirty[addr - PMEM_BASE];
      case 2: 
        for(int i =0; i < 2; i++){
          dirty |= pmem_dirty[addr - PMEM_BASE + i];
        }
        return !dirty;
      case 4:
        for(int i =0; i < 4; i++){
          dirty |= pmem_dirty[addr - PMEM_BASE + i];
        }
        return !dirty;
  #ifdef ISA64
      case 8:
        for(int i =0; i < 8; i++){
          dirty |= pmem_dirty[addr - PMEM_BASE + i];
        }
        return !dirty;
  #endif
      default: assert(0);
    }
  } else return true;
}
#endif

#ifdef DIFFTEST_STORE_COMMIT
store_commit_t store_commit_queue[STORE_QUEUE_SIZE];
static uint64_t head = 0, tail = 0;

void store_commit_queue_push(uint64_t addr, uint64_t data, int len) {
  if (cpu.amo) {
    return;
  }
  store_commit_t *commit = store_commit_queue + tail;
  assert(!commit->valid);
  uint64_t offset = addr % 8ULL;
  commit->addr = addr - offset;
  commit->valid = 1;
  switch (len) {
    case 1:
      commit->data = (data & 0xffULL) << (offset << 3);
      commit->mask = 0x1 << offset;
      break;
    case 2:
      commit->data = (data & 0xffffULL) << (offset << 3);
      commit->mask = 0x3 << offset;
      break;
    case 4:
      commit->data = (data & 0xffffffffULL) << (offset << 3);
      commit->mask = 0xf << offset;
      break;
    case 8:
      commit->data = data;
      commit->mask = 0xff;
      break;
    default:
      assert(0);
  }
  tail = (tail + 1) % STORE_QUEUE_SIZE;
}

store_commit_t *store_commit_queue_pop() {
  store_commit_t *result = store_commit_queue + head;
  if (!result->valid) {
    return NULL;
  }
  result->valid = 0;
  head = (head + 1) % STORE_QUEUE_SIZE;
  return result;
}

int check_store_commit(uint64_t *addr, uint64_t *data, uint8_t *mask) {
  *addr = *addr - (*addr % 0x8ULL);
  store_commit_t *commit = store_commit_queue_pop();
  int result = 0;
  if (!commit) {
    printf("NEMU does not commit any store instruction.\n");
    result = 1;
  }
  else if (*addr != commit->addr || *data != commit->data || *mask != commit->mask) {
    *addr = commit->addr;
    *data = commit->data;
    *mask = commit->mask;
    result = 1;
  }
  return result;
}

#endif

word_t vaddr_mmu_read(vaddr_t addr, int len, int type);
void vaddr_mmu_write(vaddr_t addr, word_t data, int len);

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

def_vaddr_template(1)
def_vaddr_template(2)
def_vaddr_template(4)
#ifdef ISA64
def_vaddr_template(8)
#endif
