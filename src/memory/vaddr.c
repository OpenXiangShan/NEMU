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

#include "common.h"
#include <isa.h>
//#include <profiling/betapoint-ext.h>
#include <profiling/profiling_control.h>

#ifdef CONFIG_PERF_OPT
#define ENABLE_HOSTTLB 1
#endif

#include <memory/paddr.h>
#include <memory/vaddr.h>
#include <memory/host-tlb.h>
#include <cpu/decode.h>

void isa_mmio_misalign_data_addr_check(paddr_t paddr, vaddr_t vaddr, int len, int type, int is_cross_page);

static paddr_t vaddr_trans_and_check_exception(vaddr_t vaddr, int len, int type, bool* exp) {
  paddr_t mmu_ret = isa_mmu_translate(vaddr, len, type);
  *exp = (mmu_ret & PAGE_MASK) != MEM_RET_OK;
  paddr_t paddr = (mmu_ret & ~PAGE_MASK) | (vaddr & PAGE_MASK);
  if (*exp) {
    return 0;
  }
  if (cpu.pbmt != 0) {
    isa_mmio_misalign_data_addr_check(paddr, vaddr, len, type, true);
  }
  *exp = !check_paddr(paddr, len, type, type, cpu.mode, vaddr);
  return paddr;
}

static word_t vaddr_read_cross_page(vaddr_t addr, int len, int type, bool needTranslate) {
  vaddr_t vaddr = addr;
  word_t data = 0;
  int i;
  for (i = 0; i < len; i ++, vaddr ++) {
    paddr_t paddr = vaddr;
    if (needTranslate) {
      paddr_t mmu_ret = isa_mmu_translate(vaddr, 1, type);
      int ret = mmu_ret & PAGE_MASK;
      if (ret != MEM_RET_OK) return 0;
      paddr = (mmu_ret & ~PAGE_MASK) | (vaddr & PAGE_MASK);
    }

#ifdef CONFIG_MULTICORE_DIFF
    word_t byte = (type == MEM_TYPE_IFETCH ? golden_pmem_read(paddr, 1) : paddr_read(paddr, 1, type, type, cpu.mode | CROSS_PAGE_LD_FLAG, vaddr));
#else
    word_t byte = paddr_read(paddr, 1, type, type, cpu.mode | CROSS_PAGE_LD_FLAG, vaddr);
#endif
    data |= byte << (i << 3);
  }
  return data;
}

static void vaddr_write_cross_page(vaddr_t addr, int len, word_t data, bool needTranslate) {
  // (unaligned & cross page) store, align with dut(xs)
  //                  4KB|
  // +---+---+---+---+---+---+---+---+---+---+---+---+
  // |   sd    *   *   *   *   *   *   *   *         |
  // +---+---+---+---+---+---+---+---+---+---+---+---+
  //  split to:
  //                  4KB|
  // +---+---+---+---+---+---+---+---+---+---+---+---+
  // | store1              *   *   *   *   *         | current page
  // +---+---+---+---+---+---+---+---+---+---+---+---+
  // +---+---+---+---+---+---+---+---+---+---+---+---+
  // | store2  *   *   *                             | next page
  // +---+---+---+---+---+---+---+---+---+---+---+---+

  vaddr_t cur_pg_st_vaddr = addr;
  vaddr_t next_pg_st_vaddr = (addr & ~PAGE_MASK) + PAGE_SIZE;
  int cur_pg_st_len = next_pg_st_vaddr - cur_pg_st_vaddr;
  int next_pg_st_len = len - cur_pg_st_len;
  word_t cur_pg_st_mask = 0;
  int i = 0;
  for (; i < cur_pg_st_len; i++) {
    cur_pg_st_mask = (cur_pg_st_mask << 8) | 0xffUL;
  }
  word_t cur_pg_st_data = data & cur_pg_st_mask;
  word_t next_pg_st_data = data >> (cur_pg_st_len << 3);
  if (needTranslate) {
    Logm("vaddr_write_cross_page!");
    // make sure no page fault or access fault before real write
    bool cur_pg_st_exp = false;
    bool next_pg_st_exp = false;
    paddr_t cur_pg_st_paddr = vaddr_trans_and_check_exception(cur_pg_st_vaddr, cur_pg_st_len, MEM_TYPE_WRITE, &cur_pg_st_exp);
    paddr_t next_pg_st_paddr = vaddr_trans_and_check_exception(next_pg_st_vaddr, next_pg_st_len, MEM_TYPE_WRITE, &next_pg_st_exp);

    if (!cur_pg_st_exp && !next_pg_st_exp) {
      paddr_write(cur_pg_st_paddr, cur_pg_st_len, cur_pg_st_data, cpu.mode | CROSS_PAGE_ST_FLAG, cur_pg_st_vaddr);
      paddr_write(next_pg_st_paddr, next_pg_st_len, next_pg_st_data, cpu.mode | CROSS_PAGE_ST_FLAG, next_pg_st_vaddr);
    }
  } else {
    paddr_write(cur_pg_st_vaddr, cur_pg_st_len, cur_pg_st_data, cpu.mode | CROSS_PAGE_ST_FLAG, cur_pg_st_vaddr);
    paddr_write(next_pg_st_vaddr, next_pg_st_len, next_pg_st_data, cpu.mode | CROSS_PAGE_ST_FLAG, next_pg_st_vaddr);
  }

}


#ifndef ENABLE_HOSTTLB

__attribute__((noinline))
static word_t vaddr_mmu_read(struct Decode *s, vaddr_t addr, int len, int type) {
  vaddr_t vaddr = addr;
  paddr_t pg_base = isa_mmu_translate(addr, len, type);
  int ret = pg_base & PAGE_MASK;
  if (ret == MEM_RET_OK) {
    addr = pg_base | (addr & PAGE_MASK);
#ifdef CONFIG_MULTICORE_DIFF
    word_t rdata = (type == MEM_TYPE_IFETCH ? golden_pmem_read(addr, len) : paddr_read(addr, len, type, type, cpu.mode, vaddr));
#else
    word_t rdata = paddr_read(addr, len, type, type, cpu.mode, vaddr);
#endif // CONFIG_MULTICORE_DIFF
#ifdef CONFIG_SHARE
    if (unlikely(dynamic_config.debug_difftest)) {
      fprintf(stderr, "[NEMU] mmu_read: vaddr 0x%lx, paddr 0x%lx, rdata 0x%lx\n",
        vaddr, addr, rdata);
    }
#endif // CONFIG_SHARE
    return rdata;
  }
  return 0;
}

__attribute__((noinline))
static void vaddr_mmu_write(struct Decode *s, vaddr_t addr, int len, word_t data) {
  vaddr_t vaddr = addr;
  paddr_t pg_base = isa_mmu_translate(addr, len, MEM_TYPE_WRITE);
  int ret = pg_base & PAGE_MASK;
  if (ret == MEM_RET_OK) {
    addr = pg_base | (addr & PAGE_MASK);
#ifdef CONFIG_SHARE
    if (unlikely(dynamic_config.debug_difftest)) {
      fprintf(stderr, "[NEMU] mmu_write: vaddr 0x%lx, paddr 0x%lx, len %d, data 0x%lx\n",
        vaddr, addr, len, data);
    }
#endif // CONFIG_SHARE
    paddr_write(addr, len, data, cpu.mode, vaddr);
  }
}

#endif // ENABLE_HOSTTLB

static inline word_t vaddr_read_internal(void *s, vaddr_t addr, int len, int type, int mmu_mode) {

#ifdef CONFIG_RVH
  // check whether here is a hlvx instruction
  // when inst fetch or vaddr_read_safe (for examine memory), s is NULL
  if (s != NULL) {
    extern int rvh_hlvx_check(struct Decode *s, int type);
    rvh_hlvx_check((Decode*)s, type);
  }
#endif

  addr = get_effective_address(addr, type);

  bool is_cross_page = false;
  void isa_misalign_data_addr_check(vaddr_t vaddr, int len, int type);
  if (type != MEM_TYPE_IFETCH) {
    isa_misalign_data_addr_check(addr, len, type);
    is_cross_page = ((addr & PAGE_MASK) + len) > PAGE_SIZE && len != 1;
  }

  if (unlikely(mmu_mode == MMU_DYNAMIC || mmu_mode == MMU_TRANSLATE)) {
    Logm("Checking mmu when MMU_DYN");
    mmu_mode = isa_mmu_check(addr, len, type);
  }

  if (is_cross_page) {
    return vaddr_read_cross_page(addr, len, type, mmu_mode == MMU_DYNAMIC || mmu_mode == MMU_TRANSLATE);
  }
  if (mmu_mode == MMU_DIRECT) {
    Logm("Paddr reading directly");
    return paddr_read(addr, len, type, type, cpu.mode, addr);
  }
  return MUXDEF(ENABLE_HOSTTLB, hosttlb_read, vaddr_mmu_read) ((struct Decode *)s, addr, len, type);
  return 0;

}

#ifdef CONFIG_RVV
extern void dummy_hosttlb_translate(struct Decode *s, vaddr_t vaddr, int len, bool is_write);

void dummy_vaddr_data_read(struct Decode *s, vaddr_t addr, int len, int mmu_mode) {

  addr = get_effective_address(addr, MEM_TYPE_READ);

  assert(!ISDEF(CONFIG_SHARE));
  if (unlikely(mmu_mode == MMU_DYNAMIC || mmu_mode == MMU_TRANSLATE)) {
    Logm("Checking mmu when MMU_DYN for dummy read");
    mmu_mode = isa_mmu_check(addr, len, MEM_TYPE_READ);
  }

  if (mmu_mode == MMU_DIRECT) {
    return;
  }
  if (ISDEF(ENABLE_HOSTTLB)) {
    dummy_hosttlb_translate(s, addr, len, false);
  }
}
#endif // CONFIG_RVV

word_t vaddr_ifetch(vaddr_t addr, int len) {
  return vaddr_read_internal(NULL, addr, len, MEM_TYPE_IFETCH, MMU_DYNAMIC);
}

word_t vaddr_read(struct Decode *s, vaddr_t addr, int len, int mmu_mode) {
  Logm("Reading vaddr %lx", addr);
  return vaddr_read_internal(s, addr, len, MEM_TYPE_READ, mmu_mode);
}

#ifdef CONFIG_RVV
void dummy_vaddr_write(struct Decode *s, vaddr_t addr, int len, int mmu_mode) {

  addr = get_effective_address(addr, MEM_TYPE_WRITE);

  assert(!ISDEF(CONFIG_SHARE));
  if (unlikely(mmu_mode == MMU_DYNAMIC || mmu_mode == MMU_TRANSLATE)) {
    mmu_mode = isa_mmu_check(addr, len, MEM_TYPE_WRITE);
  }
  if (mmu_mode == MMU_DIRECT) {
    return;
  }
  if (ISDEF(ENABLE_HOSTTLB)) {
    dummy_hosttlb_translate(s, addr, len, true);
  }
}
#endif // CONFIG_RVV

void vaddr_write(struct Decode *s, vaddr_t addr, int len, word_t data, int mmu_mode) {

  addr = get_effective_address(addr, MEM_TYPE_WRITE);

  void isa_misalign_data_addr_check(vaddr_t vaddr, int len, int type);
  isa_misalign_data_addr_check(addr, len, MEM_TYPE_WRITE);

  bool is_cross_page = ((addr & PAGE_MASK) + len) > PAGE_SIZE && len != 1;

  if (unlikely(mmu_mode == MMU_DYNAMIC || mmu_mode == MMU_TRANSLATE)) {
    mmu_mode = isa_mmu_check(addr, len, MEM_TYPE_WRITE);
  }

  if (is_cross_page) {
    vaddr_write_cross_page(addr, len ,data, mmu_mode == MMU_DYNAMIC || mmu_mode == MMU_TRANSLATE);
    return;
  }
  if (mmu_mode == MMU_DIRECT) {
    paddr_write(addr, len, data, cpu.mode, addr);
    return;
  }
  MUXDEF(ENABLE_HOSTTLB, hosttlb_write, vaddr_mmu_write) (s, addr, len, data);

}

word_t vaddr_read_safe(vaddr_t addr, int len) {
  // FIXME: when reading fails, return an error instead of raising exceptions
  return vaddr_read_internal(NULL, addr, len, MEM_TYPE_READ, MMU_DYNAMIC);
}
