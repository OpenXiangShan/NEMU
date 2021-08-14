#ifndef __MEMORY_PADDR_H__
#define __MEMORY_PADDR_H__

#include <common.h>
#ifdef CONFIG_MODE_USER
#define CONFIG_MBASE 0
#define CONFIG_MSIZE 0
#define CONFIG_PC_RESET_OFFSET 0
#endif

#define RESET_VECTOR (CONFIG_MBASE + CONFIG_PC_RESET_OFFSET)

/* convert the guest physical address in the guest program to host virtual address in NEMU */
uint8_t* guest_to_host(paddr_t paddr);
/* convert the host virtual address in NEMU to guest physical address in the guest program */
paddr_t host_to_guest(uint8_t *haddr);

static inline bool in_pmem(paddr_t addr) {
#ifndef __ICS_EXPORT
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
#else
  return (addr >= CONFIG_MBASE) && (addr < (paddr_t)CONFIG_MBASE + CONFIG_MSIZE);
#endif
}

word_t paddr_read(paddr_t addr, int len);
void paddr_write(paddr_t addr, int len, word_t data);

#endif
