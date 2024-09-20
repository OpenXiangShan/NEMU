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

#ifndef __ICS_EXPORT
#ifndef ENABLE_HOSTTLB

static paddr_t vaddr_trans_and_check_exception(vaddr_t vaddr, int len, int type, bool* exp) {
  paddr_t mmu_ret = isa_mmu_translate(vaddr & ~PAGE_MASK, len, type);
  *exp = (mmu_ret & PAGE_MASK) != MEM_RET_OK;
  paddr_t paddr = (mmu_ret & ~PAGE_MASK) | (vaddr & PAGE_MASK);
  if (*exp) {
    return 0;
  }
  *exp = !check_paddr(paddr, len, type, type, cpu.mode, vaddr);
  return paddr;
}

static word_t vaddr_read_cross_page(vaddr_t addr, int len, int type) {
  vaddr_t vaddr = addr;
  word_t data = 0;
  int i;
  for (i = 0; i < len; i ++, addr ++) {
    paddr_t mmu_ret = isa_mmu_translate(addr, 1, type);
    int ret = mmu_ret & PAGE_MASK;
    if (ret != MEM_RET_OK) return 0;
    paddr_t paddr = (mmu_ret & ~PAGE_MASK) | (addr & PAGE_MASK);
#ifdef CONFIG_MULTICORE_DIFF
    word_t byte = (type == MEM_TYPE_IFETCH ? golden_pmem_read(paddr, 1, type, cpu.mode | CROSS_PAGE_LD_FLAG, vaddr) : paddr_read(paddr, 1, type, type, cpu.mode | CROSS_PAGE_LD_FLAG, vaddr));
#else
    word_t byte = (type == MEM_TYPE_IFETCH ? paddr_read : paddr_read)(paddr, 1, type, type, cpu.mode | CROSS_PAGE_LD_FLAG, vaddr);
#endif
    data |= byte << (i << 3);
  }
  return data;
}

static void vaddr_write_cross_page(vaddr_t addr, int len, word_t data) {
  Log("vaddr_write_cross_page!");
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
  // make sure no page fault or access fault before real write
  bool cur_pg_st_exp = false;
  bool next_pg_st_exp = false;
  paddr_t cur_pg_st_paddr = vaddr_trans_and_check_exception(cur_pg_st_vaddr, cur_pg_st_len, MEM_TYPE_WRITE, &cur_pg_st_exp);
  paddr_t next_pg_st_paddr = vaddr_trans_and_check_exception(next_pg_st_vaddr, next_pg_st_len, MEM_TYPE_WRITE, &next_pg_st_exp);
  
  if (!cur_pg_st_exp && !next_pg_st_exp) {
    paddr_write(cur_pg_st_paddr, cur_pg_st_len, cur_pg_st_data, cpu.mode | CROSS_PAGE_ST_FLAG, cur_pg_st_vaddr);
    paddr_write(next_pg_st_paddr, next_pg_st_len, next_pg_st_data, cpu.mode | CROSS_PAGE_ST_FLAG, next_pg_st_vaddr);
  }
}

__attribute__((noinline))
static word_t vaddr_mmu_read(struct Decode *s, vaddr_t addr, int len, int type) {
  vaddr_t vaddr = addr;
  paddr_t pg_base = isa_mmu_translate(addr, len, type);
  int ret = pg_base & PAGE_MASK;
  if (ret == MEM_RET_OK) {
    addr = pg_base | (addr & PAGE_MASK);
#ifdef CONFIG_MULTICORE_DIFF
    word_t rdata = (type == MEM_TYPE_IFETCH ? golden_pmem_read(addr, len, type, cpu.mode, vaddr) : paddr_read(addr, len, type, type, cpu.mode, vaddr));
#else
    word_t rdata = paddr_read(addr, len, type, type, cpu.mode, vaddr);
#endif
#ifdef CONFIG_SHARE
    if (unlikely(dynamic_config.debug_difftest)) {
      fprintf(stderr, "[NEMU] mmu_read: vaddr 0x%lx, paddr 0x%lx, rdata 0x%lx\n",
        vaddr, addr, rdata);
    }
#endif
    return rdata;
  } else if (len != 1 && ret == MEM_RET_CROSS_PAGE) {
    return vaddr_read_cross_page(addr, len, type);
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
#endif
    paddr_write(addr, len, data, cpu.mode, vaddr);
  } else if (len != 1 && ret == MEM_RET_CROSS_PAGE) {
    vaddr_write_cross_page(addr, len, data);
  }
}
#endif
#endif

static inline word_t vaddr_read_internal(void *s, vaddr_t addr, int len, int type, int mmu_mode) {
  void isa_misalign_data_addr_check(vaddr_t vaddr, int len, int type);
  if (type != MEM_TYPE_IFETCH) {
    isa_misalign_data_addr_check(addr, len, type);
  }
  if (unlikely(mmu_mode == MMU_DYNAMIC || mmu_mode == MMU_TRANSLATE)) {
    Logm("Checking mmu when MMU_DYN");
    mmu_mode = isa_mmu_check(addr, len, type);
  }
  if (mmu_mode == MMU_DIRECT) {
    Logm("Paddr reading directly");
    return paddr_read(addr, len, type, type, cpu.mode, addr);
  }
#ifndef __ICS_EXPORT
#ifdef CONFIG_RVH
  if(type != MEM_TYPE_IFETCH){
    extern int rvh_hlvx_check(struct Decode *s, int type);
    rvh_hlvx_check(s, type);
  }
#endif
  return MUXDEF(ENABLE_HOSTTLB, hosttlb_read, vaddr_mmu_read) ((struct Decode *)s, addr, len, type);
#endif
  return 0;
}

#ifdef CONFIG_RVV
extern void dummy_hosttlb_translate(struct Decode *s, vaddr_t vaddr, int len, bool is_write);

void dummy_vaddr_data_read(struct Decode *s, vaddr_t addr, int len, int mmu_mode) {
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

extern uint64_t  vld_flag;
extern uint64_t  vst_flag;

extern Decode *prev_s;

word_t vaddr_ifetch(vaddr_t addr, int len) {
  return vaddr_read_internal(NULL, addr, len, MEM_TYPE_IFETCH, MMU_DYNAMIC);
}

word_t vaddr_read(struct Decode *s, vaddr_t addr, int len, int mmu_mode) {
  Logm("Reading vaddr %lx", addr);
  if (vld_flag == 1){
    printf("[NEMU] pc: 0x%lx, instr: 0x%x, vaddr read addr:" FMT_PADDR ", len:%d\n",prev_s->pc, prev_s->isa.instr.val, addr, len);
  }
  return vaddr_read_internal(s, addr, len, MEM_TYPE_READ, mmu_mode);
}

#ifdef CONFIG_RVV
void dummy_vaddr_write(struct Decode *s, vaddr_t addr, int len, int mmu_mode) {
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
  if (vst_flag == 1){
    printf("[NEMU] pc: 0x%lx, instr: 0x%x, vaddr write addr:" FMT_PADDR ", data: %016lx, len:%d\n",prev_s->pc, prev_s->isa.instr.val, addr, data, len);
  }
  void isa_misalign_data_addr_check(vaddr_t vaddr, int len, int type);
  isa_misalign_data_addr_check(addr, len, MEM_TYPE_WRITE);
  if (unlikely(mmu_mode == MMU_DYNAMIC || mmu_mode == MMU_TRANSLATE)) {
    mmu_mode = isa_mmu_check(addr, len, MEM_TYPE_WRITE);
  }
  if (mmu_mode == MMU_DIRECT) {
    paddr_write(addr, len, data, cpu.mode, addr);
    return;
  }
#ifndef __ICS_EXPORT
  MUXDEF(ENABLE_HOSTTLB, hosttlb_write, vaddr_mmu_write) (s, addr, len, data);
#endif
}

word_t vaddr_read_safe(vaddr_t addr, int len) {
  // FIXME: when reading fails, return an error instead of raising exceptions
  return vaddr_read_internal(NULL, addr, len, MEM_TYPE_READ, MMU_DYNAMIC);
}
