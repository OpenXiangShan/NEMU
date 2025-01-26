/***************************************************************************************
* Copyright (c) 2014-2021 Zihao Yu, Nanjing University
* Copyright (c) 2020-2022 Institute of Computing Technology, Chinese Academy of Sciences
*
* NEMU is licensed under Mulan PSL v2.
* You can use this software according to the terms and conditions of the Mulan PSL v2.
* You may obtain a copy of Mulan PSL v2 at:
*          http://license.coscl.org.cn/MulanPSL2
*
* THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND,
* EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT,
* MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE.
*
* See the Mulan PSL v2 for more details.
***************************************************************************************/

#ifndef __MEMORY_PADDR_H__
#define __MEMORY_PADDR_H__

#include <common.h>

extern unsigned long MEMORY_SIZE;
extern unsigned int PMEM_HARTID;

#ifdef CONFIG_MODE_USER
#define CONFIG_MBASE 0
#define CONFIG_MSIZE 0
#define CONFIG_PC_RESET_OFFSET 0
#endif

/* Used to indicate whether the write is non-aligned across pages write. Reuse this flag inside mode*/
#define CROSS_PAGE_ST_SHIFT 12
#define CROSS_PAGE_ST_FLAG  (1u << CROSS_PAGE_ST_SHIFT)
#define CROSS_PAGE_LD_SHIFT 12
#define CROSS_PAGE_LD_FLAG  (1u << CROSS_PAGE_LD_SHIFT)

#define RESET_VECTOR (CONFIG_MBASE + CONFIG_PC_RESET_OFFSET)

void init_mem();

/* convert the guest physical address in the guest program to host virtual address in NEMU */
uint8_t* guest_to_host(paddr_t paddr);
/* convert the host virtual address in NEMU to guest physical address in the guest program */
paddr_t host_to_guest(uint8_t *haddr);

static inline bool in_pmem(paddr_t addr) {
  paddr_t mbase_mask = CONFIG_MBASE - 1;
  paddr_t msize_mask = MEMORY_SIZE - 1;
  bool mbase_align = (CONFIG_MBASE & mbase_mask) == 0;
  bool msize_align = (MEMORY_SIZE & msize_mask) == 0;
  bool msize_inside_mbase = msize_mask <= mbase_mask;
  if (mbase_align && msize_align && msize_inside_mbase) {
    return (addr & ~msize_mask) == CONFIG_MBASE;
  } else {
    #ifdef CONFIG_USE_SPARSEMM
    return addr >= CONFIG_MBASE;
    #else
    return (addr >= CONFIG_MBASE) && (addr < (paddr_t)CONFIG_MBASE + MEMORY_SIZE);
    #endif
  }
}

word_t paddr_read(paddr_t addr, int len, int type, int trap_type, int mode, vaddr_t vaddr);
void paddr_write(paddr_t addr, int len, word_t data, int mode, vaddr_t vaddr);
bool check_paddr(paddr_t addr, int len, int type, int trap_type, int mode, vaddr_t vaddr);
#ifdef CONFIG_RV_MBMC
word_t bitmap_read(paddr_t addr, int type, int mode);
#endif
uint8_t *get_pmem();

#if CONFIG_ENABLE_MEM_DEDUP || CONFIG_USE_MMAP
/** Currently, when enable mmap, memory allocation is done by NEMU itself.
  * If one wants to control when is memory allocated,
  * he/she can disable default alloction in NEMU and call set_pmem(fales, NULL) from outside.
  */
void set_pmem(bool pass_pmem_from_dut, uint8_t *_pmem);
#endif


#ifdef CONFIG_USE_SPARSEMM
void * get_sparsemm();
#endif

#ifdef CONFIG_STORE_LOG
typedef struct {
#ifdef CONFIG_LIGHTQS
  uint64_t inst_cnt;
#endif // CONFIG_LIGHTQS
  paddr_t addr;
  word_t orig_data;
  // new value and write length makes no sense for restore
} store_log_t;
#endif // CONFIG_STORE_LOG

#ifdef CONFIG_DIFFTEST_STORE_COMMIT

typedef struct {
    uint64_t addr;
    uint64_t data;
    uint8_t  mask;
    uint64_t pc;
} store_commit_t;

/**
 * In the implementation, CPP queue is used for store commit maintenance.
 * */
void store_commit_queue_push(uint64_t addr, uint64_t data, int len, int cross_page_store);

/**
 * Check whether there are valid entries.
 * If there are valid entries, the queue exits normally and returns.
 * The return result is not guaranteed when there are no valid entries.
 * @param flag Returns a result valid flag, where 1 is valid.
 * @return store_commit_t struct
 */
store_commit_t store_commit_queue_pop(int *flag);
int check_store_commit(uint64_t *addr, uint64_t *data, uint8_t *mask);

store_commit_t get_store_commit_info();
#endif

//#define CONFIG_MEMORY_REGION_ANALYSIS
#ifdef CONFIG_MEMORY_REGION_ANALYSIS
#include <math.h>
#define PROGRAM_MEMORY_SIZE (CONFIG_MSIZE /1024 /1024) // MB
#define PROGRAM_ANALYSIS_PAGES (PROGRAM_MEMORY_SIZE / CONFIG_MEMORY_REGION_ANALYSIS_SIZE)
#define ALIGNMENT_SIZE ((int)log2(CONFIG_MEMORY_REGION_ANALYSIS_SIZE * 1024 * 1024))// set MB alig

void analysis_memory_commit(uint64_t addr);
void analysis_use_addr_display();
bool analysis_memory_isuse(uint64_t page);
#endif

#ifdef CONFIG_MULTICORE_DIFF
extern uint8_t* golden_pmem;

static inline word_t golden_pmem_read(paddr_t addr, int len, int type, int mode, vaddr_t vaddr) {
  assert(golden_pmem != NULL);
  mode &= ~CROSS_PAGE_LD_FLAG;
#ifdef CONFIG_USE_SPARSEMM
  return sparse_mem_wread((void *)golden_pmem, addr, len)
#else
  void *p = &golden_pmem[addr - 0x80000000];
  switch (len) {
    case 1: return *(uint8_t  *)p;
    case 2: return *(uint16_t *)p;
    case 4: return *(uint32_t *)p;
    case 8: return *(uint64_t *)p;
    default: assert(0);
  }
#endif
}
#endif

#endif
