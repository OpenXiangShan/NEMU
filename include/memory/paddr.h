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
  paddr_t msize_mask = MEMORY_SIZE - 1;
  bool mbase_align = (CONFIG_MBASE & mbase_mask) == 0;
  bool msize_align = (MEMORY_SIZE & msize_mask) == 0;
  bool msize_inside_mbase = msize_mask <= mbase_mask;
  if (mbase_align && msize_align && msize_inside_mbase) {
    return (addr & ~msize_mask) == CONFIG_MBASE;
  } else {
    return (addr >= CONFIG_MBASE) && (addr < (paddr_t)CONFIG_MBASE + MEMORY_SIZE);
  }
}

word_t paddr_read(paddr_t addr, int len, int type, int mode, vaddr_t vaddr);
void paddr_write(paddr_t addr, int len, word_t data, int mode, vaddr_t vaddr);
uint8_t *get_pmem();

#ifdef CONFIG_DIFFTEST_STORE_COMMIT

#define STORE_QUEUE_SIZE 64
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

static inline word_t golden_pmem_read(paddr_t addr, int len, int type, int mode, vaddr_t vaddr) {
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
