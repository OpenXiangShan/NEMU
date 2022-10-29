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

#include <isa.h>
#include <memory/host.h>
#include <memory/paddr.h>
#include <device/mmio.h>
#include <stdlib.h>
#include <time.h>
#include <cpu/cpu.h>
#include "../local-include/csr.h"
#include "../local-include/intr.h"

bool is_in_mmio(paddr_t addr);

unsigned long MEMORY_SIZE = CONFIG_MSIZE;

#ifdef CONFIG_USE_MMAP
#include <sys/mman.h>
static uint8_t *pmem = (uint8_t *)0x100000000ul;
#else
static uint8_t pmem[CONFIG_MSIZE] PG_ALIGN = {};
#endif
#define HOST_PMEM_OFFSET (uint8_t *)(pmem - CONFIG_MBASE)

uint8_t *get_pmem()
{
  return pmem;
}

uint8_t* guest_to_host(paddr_t paddr) { return paddr + HOST_PMEM_OFFSET; }
paddr_t host_to_guest(uint8_t *haddr) { return haddr - HOST_PMEM_OFFSET; }

static inline word_t pmem_read(paddr_t addr, int len) {
  return host_read(guest_to_host(addr), len);
}

static inline void pmem_write(paddr_t addr, int len, word_t data) {
#ifdef CONFIG_DIFFTEST_STORE_COMMIT
  store_commit_queue_push(addr, data, len);
#endif
  host_write(guest_to_host(addr), len, data);
}

static inline void raise_access_fault(int cause, vaddr_t vaddr) {
  INTR_TVAL_REG(cause) = vaddr;
  // cpu.amo flag must be reset to false before longjmp_exception,
  // including longjmp_exception(access fault), longjmp_exception(page fault)
  cpu.amo = false;
  longjmp_exception(cause);
}

static inline void raise_read_access_fault(int type, vaddr_t vaddr) {
  int cause = EX_LAF;
  if (type == MEM_TYPE_IFETCH || type == MEM_TYPE_IFETCH_READ) { cause = EX_IAF; }
  else if (cpu.amo || type == MEM_TYPE_WRITE_READ)             { cause = EX_SAF; }
  raise_access_fault(cause, vaddr);
}

void init_mem() {
#ifdef CONFIG_USE_MMAP
  #ifdef CONFIG_MULTICORE_DIFF
    panic("Pmem must not use mmap during multi-core difftest");
  #endif
  void *ret = mmap((void *)pmem, MEMORY_SIZE, PROT_READ | PROT_WRITE,
      MAP_ANONYMOUS | MAP_PRIVATE | MAP_FIXED, -1, 0);
  if (ret != pmem) {
    perror("mmap");
    assert(0);
  }
#endif

#ifdef CONFIG_DIFFTEST_STORE_COMMIT
  for (int i = 0; i < STORE_QUEUE_SIZE; i++) {
    store_commit_queue[i].valid = 0;
  }
#endif

#ifdef CONFIG_MEM_RANDOM
  srand(time(0));
  uint32_t *p = (uint32_t *)pmem;
  int i;
  for (i = 0; i < (int) (MEMORY_SIZE / sizeof(p[0])); i ++) {
    p[i] = rand();
  }
#endif
}

/* Memory accessing interfaces */

word_t paddr_read(paddr_t addr, int len, int type, int mode, vaddr_t vaddr) {


  assert(type == MEM_TYPE_READ || type == MEM_TYPE_IFETCH_READ || type == MEM_TYPE_IFETCH || type == MEM_TYPE_WRITE_READ);
  if (!isa_pmp_check_permission(addr, len, type, mode)) {
    Log("isa pmp check failed");
    raise_read_access_fault(type, vaddr);
    return 0;
  }
#ifndef CONFIG_SHARE
  if (likely(in_pmem(addr))) return pmem_read(addr, len);
  else {
    if (likely(is_in_mmio(addr))) return mmio_read(addr, len);
    else raise_read_access_fault(type, vaddr);
    return 0;
  }
#else
  if (likely(in_pmem(addr))) {
    uint64_t rdata = pmem_read(addr, len);
    if (dynamic_config.debug_difftest) {
      fprintf(stderr, "[NEMU] paddr read addr:" FMT_PADDR ", data: %016lx, len:%d, type:%d, mode:%d\n",
        addr, rdata, len, type, mode);
    }
    return rdata;
  }
  else {
#ifdef CONFIG_HAS_FLASH
    if (likely(is_in_mmio(addr))) return mmio_read(addr, len);
#endif
    if(dynamic_config.ignore_illegal_mem_access)
      return 0;
    printf("ERROR: invalid mem read from paddr " FMT_PADDR ", NEMU raise access exception\n", addr);
    raise_read_access_fault(type, vaddr);
  }
  return 0;
#endif
}

void paddr_write(paddr_t addr, int len, word_t data, int mode, vaddr_t vaddr) {
  if (!isa_pmp_check_permission(addr, len, MEM_TYPE_WRITE, mode)) {
    raise_access_fault(EX_SAF, vaddr);
    return ;
  }
#ifndef CONFIG_SHARE
  if (likely(in_pmem(addr))) pmem_write(addr, len, data);
  else {
    if (likely(is_in_mmio(addr))) mmio_write(addr, len, data);
    else raise_access_fault(EX_SAF, vaddr);
  }
#else
  if (likely(in_pmem(addr))) {
    if(dynamic_config.debug_difftest) {
      fprintf(stderr, "[NEMU] paddr write addr:" FMT_PADDR ", data:%016lx, len:%d, mode:%d\n",
        addr, data, len, mode);
    }
    return pmem_write(addr, len, data);
  }
  else {
    if(dynamic_config.ignore_illegal_mem_access)
      return;
    printf("ERROR: invalid mem write to paddr " FMT_PADDR ", NEMU raise access exception\n", addr);
    raise_access_fault(EX_SAF, vaddr);
    return;
  }
#endif
}


#ifdef CONFIG_DIFFTEST_STORE_COMMIT
store_commit_t store_commit_queue[STORE_QUEUE_SIZE];
static uint64_t head = 0, tail = 0;

void store_commit_queue_push(uint64_t addr, uint64_t data, int len) {
  if (cpu.amo) {
    return;
  }
  static int overflow = 0;
  store_commit_t *commit = store_commit_queue + tail;
  if (commit->valid && !overflow) { // store commit queue overflow
    overflow = 1;
    printf("[WARNING] difftest store queue overflow\n");
  };
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
