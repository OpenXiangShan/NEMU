#include <memory/host.h>
#include <memory/paddr.h>
#include <device/mmio.h>
#include <isa.h>

#if defined(CONFIG_USE_MMAP)
#include <sys/mman.h>
#define pmem ((uint8_t *)0x100000000ul)
#elif defined(CONFIG_TARGET_AM)
static uint8_t *pmem = NULL;
#else
static uint8_t pmem[CONFIG_MSIZE] PG_ALIGN = {};
#endif

uint8_t* guest_to_host(paddr_t paddr) { return pmem + paddr - CONFIG_MBASE; }
paddr_t host_to_guest(uint8_t *haddr) { return haddr - pmem + CONFIG_MBASE; }

static word_t pmem_read(paddr_t addr, int len) {
  word_t ret = host_read(guest_to_host(addr), len);
#if defined(CONFIG_MTRACE_COND)
  if (MTRACE_COND) log_write(FMT_WORD ": pmem[" FMT_PADDR "] %d-> " FMT_WORD "\n",
      cpu.pc, addr, len, ret);
#endif
  return ret;
}

static void pmem_write(paddr_t addr, int len, word_t data) {
  host_write(guest_to_host(addr), len, data);
#if defined(CONFIG_MTRACE_COND)
  if (MTRACE_COND) log_write(FMT_WORD ": pmem[" FMT_PADDR "] <-%d " FMT_WORD "\n",
      cpu.pc, addr, len, data);
#endif
}

void init_mem() {
#if defined(CONFIG_USE_MMAP)
  void *ret = mmap((void *)pmem, CONFIG_MSIZE, PROT_READ | PROT_WRITE,
      MAP_ANONYMOUS | MAP_PRIVATE | MAP_FIXED, -1, 0);
  if (ret != pmem) {
    perror("mmap");
    assert(0);
  }
#elif defined(CONFIG_TARGET_AM)
  pmem = malloc(CONFIG_MSIZE);
  assert(pmem);
#endif
#ifdef CONFIG_MEM_RANDOM
  uint32_t *p = (uint32_t *)pmem;
  int i;
  for (i = 0; i < (int) (CONFIG_MSIZE / sizeof(p[0])); i ++) {
    p[i] = rand();
  }
#endif
  Log("physical memory area [" FMT_PADDR ", " FMT_PADDR "]",
      (paddr_t)CONFIG_MBASE, (paddr_t)CONFIG_MBASE + CONFIG_MSIZE);
}

word_t paddr_read(paddr_t addr, int len) {
  if (likely(in_pmem(addr))) return pmem_read(addr, len);
  MUXDEF(CONFIG_DEVICE, return mmio_read(addr, len),
    panic("address = " FMT_PADDR " is out of bound of physical memory [" FMT_PADDR ", " FMT_PADDR ")",
      addr, CONFIG_MBASE, CONFIG_MBASE + CONFIG_MSIZE));
}

void paddr_write(paddr_t addr, int len, word_t data) {
  if (likely(in_pmem(addr))) { pmem_write(addr, len, data); return; }
  MUXDEF(CONFIG_DEVICE, mmio_write(addr, len, data),
    panic("address = " FMT_PADDR " is out of bound of physical memory [" FMT_PADDR ", " FMT_PADDR ")",
      addr, CONFIG_MBASE, CONFIG_MBASE + CONFIG_MSIZE));
}
