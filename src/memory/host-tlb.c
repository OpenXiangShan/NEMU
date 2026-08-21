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

#include <cpu/difftest/ame/amu_ctrl_queue_wrapper.h>
#include <ctrl/ame/cutest.h>
#include <isa.h>
#include <memory/host.h>
#include <memory/vaddr.h>
#include <memory/paddr.h>
#include <cpu/cpu.h>
#include <cpu/decode.h>
#include "../local-include/intr.h"
#ifdef CONFIG_AME_MEM_ACCESS_CHECK
#include <ame/svstore_queue_wrapper.h>
#endif // CONFIG_AME_MEM_ACCESS_CHECK

#define HOSTTLB_SIZE_SHIFT 12
#define HOSTTLB_SIZE (1 << HOSTTLB_SIZE_SHIFT)

typedef struct {
  uint8_t *offset; // offset from the guest virtual address of the data page to the host virtual address
  vaddr_t gvpn; // guest virtual page number
  uint64_t generation;
} HostTLBEntry;

static HostTLBEntry hosttlb[HOSTTLB_SIZE * 3];
static uint64_t hosttlb_generation = 1;
// dummy_hosttlb_translate assumes that write tlb is right after read tlb by 1*HOSTTLB_SIZE
static HostTLBEntry* const hostrtlb = &hosttlb[0];
static HostTLBEntry* const hostwtlb = &hosttlb[HOSTTLB_SIZE];
static HostTLBEntry* const hostxtlb = &hosttlb[HOSTTLB_SIZE * 2];

static inline vaddr_t hosttlb_vpn(vaddr_t vaddr) {
  return (vaddr >> PAGE_SHIFT);
}

static inline int hosttlb_idx(vaddr_t vaddr) {
  return (hosttlb_vpn(vaddr) % HOSTTLB_SIZE);
}

void hosttlb_flush(vaddr_t vaddr) {
  Logm("hosttlb_flush " FMT_WORD, vaddr);
  // HostTLB entries are keyed by 4 KiB VPNs but do not retain the page-table
  // leaf size. An address-specific SFENCE.VMA must also invalidate sibling
  // entries derived from a superpage translation, so flush conservatively.
  if (unlikely(++hosttlb_generation == 0)) {
    memset(hosttlb, 0, sizeof(hosttlb));
    hosttlb_generation = 1;
  }
}

void hosttlb_init() {
  hosttlb_flush(0);
}

static inline bool hosttlb_cacheable(int type) {
#ifdef CONFIG_PERF_OPT_SHARE
  return isa_hosttlb_check(type);
#else
  return true;
#endif
}

static paddr_t va2pa(struct Decode *s, vaddr_t vaddr, int len, int type) {
  if (type != MEM_TYPE_IFETCH) save_globals(s);
  paddr_t pg_base = isa_mmu_translate(vaddr, len, type);
  int ret = pg_base & PAGE_MASK;
  assert(ret == MEM_RET_OK);
  return pg_base | (vaddr & PAGE_MASK);
}

static inline word_t hosttlb_paddr_read(paddr_t paddr, int len, int type,
                                       int trap_type, vaddr_t vaddr) {
#ifdef CONFIG_MULTICORE_DIFF
  if (trap_type == MEM_TYPE_IFETCH) {
    if (!isa_pmp_check_permission(paddr, len, MEM_TYPE_IFETCH, cpu.mode) ||
        !isa_pma_check_permission(paddr, len, MEM_TYPE_IFETCH)) {
      cpu.trapInfo.tval = vaddr;
      longjmp_exception(EX_IAF);
    }
    return golden_pmem_read(paddr, len);
  }
#endif
  return paddr_read(paddr, len, type, trap_type, cpu.mode, vaddr);
}

__attribute__((noinline))
static word_t hosttlb_read_slowpath(struct Decode *s, vaddr_t vaddr, int len, int type, int trap_type) {
  paddr_t paddr = va2pa(s, vaddr, len, trap_type);
  word_t data = hosttlb_paddr_read(paddr, len, type, trap_type, vaddr);
  if (type != MEM_TYPE_READ_EXEC && hosttlb_cacheable(trap_type) &&
    MUXDEF(CONFIG_PERF_OPT_SHARE, cpu.pbmt == 0, true) &&
    MUXDEF(CONFIG_RV_MBMC, isa_bmc_check_permission(paddr, len, 0, 0), true) &&
    likely(in_pmem(paddr))
  ) {
    HostTLBEntry *e = trap_type == MEM_TYPE_IFETCH ?
      &hostxtlb[hosttlb_idx(vaddr)] : &hostrtlb[hosttlb_idx(vaddr)];
    e->offset = guest_to_host(paddr) - vaddr;
    e->gvpn = hosttlb_vpn(vaddr);
    e->generation = hosttlb_generation;
  }
  Logtr("Slowpath, vaddr " FMT_WORD " --> paddr: " FMT_PADDR, vaddr, paddr);
#ifdef CONFIG_TRACE_MATRIX_LOAD_STORE
  if (type == MEM_TYPE_MATRIX_READ) {
    fprintf(stderr, "?? slow-path hosttlb_read paddr " FMT_WORD ", len: %d, type: %d\n", paddr, len, type);
  }
#endif // CONFIG_TRACE_MATRIX_LOAD_STORE
  return data;
}

#ifdef CONFIG_RV_AME
__attribute__((noinline))
static void hosttlb_read_matrix_slowpath(struct Decode *s, vaddr_t vbase, vaddr_t stride,
                                          int row, int column, int msew, bool transpose, char m_name, int mreg_id) {
  paddr_t pbase = va2pa(s, vbase, 1 << msew, MEM_TYPE_MATRIX_READ);
  paddr_read_matrix(pbase, stride, row, column, msew, transpose, cpu.mode, vbase, m_name, mreg_id);
  if (likely(in_pmem(pbase))) {
    HostTLBEntry *e = &hostrtlb[hosttlb_idx(vbase)];
    e->offset = guest_to_host(pbase) - vbase;
    e->gvpn = hosttlb_vpn(vbase);
    e->generation = hosttlb_generation;
  }
  Logtr("Slowpath, vaddr " FMT_WORD " --> paddr: " FMT_PADDR, vbase, pbase);
#ifdef CONFIG_TRACE_MATRIX_LOAD_STORE
  fprintf(stderr, "?? slow-path hosttlb_read paddr " FMT_WORD ", len: %d, type: %d\n", pbase, 1 << msew, MEM_TYPE_MATRIX_READ);
#endif // CONFIG_TRACE_MATRIX_LOAD_STORE
}
#endif // CONFIG_RV_AME

__attribute__((noinline))
static void hosttlb_write_slowpath(struct Decode *s, vaddr_t vaddr, int len, word_t data) {
  paddr_t paddr = va2pa(s, vaddr, len, MEM_TYPE_WRITE);
  paddr_write(paddr, len, data, cpu.mode, vaddr);
  if (hosttlb_cacheable(MEM_TYPE_WRITE) &&
    MUXDEF(CONFIG_PERF_OPT_SHARE, cpu.pbmt == 0, true) &&
    MUXDEF(CONFIG_RV_MBMC, isa_bmc_check_permission(paddr, len, 0, 0), true) &&
    likely(in_pmem(paddr))
  ) {
    HostTLBEntry *e = &hostwtlb[hosttlb_idx(vaddr)];
    e->offset = guest_to_host(paddr) - vaddr;
    e->gvpn = hosttlb_vpn(vaddr);
    e->generation = hosttlb_generation;
  }
}

#ifdef CONFIG_RV_AME
__attribute__((noinline))
static void hosttlb_write_matrix_slowpath(struct Decode *s, vaddr_t vbase, vaddr_t stride,
                                          int row, int column, int msew, bool transpose, char m_name, int mreg_id) {
  paddr_t pbase = va2pa(s, vbase, 1 << msew, MEM_TYPE_MATRIX_WRITE);
  paddr_write_matrix(pbase, stride, row, column, msew, transpose, cpu.mode, vbase, m_name, mreg_id);
  if (likely(in_pmem(pbase))) {
    HostTLBEntry *e = &hostwtlb[hosttlb_idx(vbase)];
    e->offset = guest_to_host(pbase) - vbase;
    e->gvpn = hosttlb_vpn(vbase);
    e->generation = hosttlb_generation;
  }
#ifdef CONFIG_TRACE_MATRIX_LOAD_STORE
  fprintf(stderr, "?? slow-path hosttlb_write paddr " FMT_WORD ", len: %d, type: %d\n",
    pbase, 1 << msew, MEM_TYPE_MATRIX_WRITE);
#endif // CONFIG_TRACE_MATRIX_LOAD_STORE
}
#endif // CONFIG_RV_AME

word_t hosttlb_read(struct Decode *s, vaddr_t vaddr, int len, int type, int trap_type) {
  Logm("hosttlb_reading " FMT_WORD, vaddr);
#ifdef CONFIG_RV_ZICFISS
  if (cpu.shadow_stack_access) {
    paddr_t paddr = va2pa(s, vaddr, len, trap_type);
    return paddr_read(paddr, len, type, trap_type, cpu.mode, vaddr);
  }
#endif
  // A cached ordinary read does not establish HLVX execute permission.
  if (type == MEM_TYPE_READ_EXEC || !hosttlb_cacheable(trap_type)) {
    return hosttlb_read_slowpath(s, vaddr, len, type, trap_type);
  }
#ifdef CONFIG_RVH
  extern bool has_two_stage_translation();
  if(has_two_stage_translation()){
    paddr_t paddr = va2pa(s, vaddr, len, trap_type);
    return hosttlb_paddr_read(paddr, len, type, trap_type, vaddr);
  }
#endif
  vaddr_t gvpn = hosttlb_vpn(vaddr);
  HostTLBEntry *e = trap_type == MEM_TYPE_IFETCH ?
    &hostxtlb[hosttlb_idx(vaddr)] : &hostrtlb[hosttlb_idx(vaddr)];
  if (unlikely(e->generation != hosttlb_generation || e->gvpn != gvpn)) {
    Logm("Host TLB slow path");
    return hosttlb_read_slowpath(s, vaddr, len, type, trap_type);
  } else {
    Logm("Host TLB fast path");
#ifdef CONFIG_PERF_OPT_SHARE
    cpu.pbmt = 0;
    if (dynamic_config.debug_difftest || ISDEF(CONFIG_MULTICORE_DIFF) ||
        ISDEF(CONFIG_MEMORY_REGION_ANALYSIS) || ISDEF(CONFIG_AME_MEM_ACCESS_CHECK) ||
        ISDEF(CONFIG_RV_MPT_CHECK) || ISDEF(CONFIG_RV_MBMC)) {
      return hosttlb_paddr_read(host_to_guest(e->offset + vaddr), len,
                               type, trap_type, vaddr);
    }
#endif
    return host_read(e->offset + vaddr, len);
  }
}

#ifdef CONFIG_RV_AME
void hosttlb_read_matrix(struct Decode *s, vaddr_t vbase, vaddr_t stride,
                           int row, int column, int msew, bool transpose,
                           char m_name, int mreg_id) {
  Logm("hosttlb_reading_matrix " FMT_WORD, vbase);
#ifdef CONFIG_RVH
  extern bool has_two_stage_translation();
  if(has_two_stage_translation()){
    paddr_t pbase = va2pa(s, vbase, 1 << msew, MEM_TYPE_MATRIX_READ);
#ifdef CONFIG_TRACE_MATRIX_LOAD_STORE
    fprintf(stderr, "?? 2-stage hosttlb_read paddr " FMT_WORD ", len: %d, type: %d\n", paddr, 1 << msew, type);
#endif // CONFIG_TRACE_MATRIX_LOAD_STORE
    paddr_read_matrix(pbase, stride, row, column, msew, transpose, cpu.mode, vbase, m_name, mreg_id);
  } else
#endif
  {
    vaddr_t gvpn = hosttlb_vpn(vbase);
    HostTLBEntry *e = &hostrtlb[hosttlb_idx(vbase)];
    if (unlikely(e->generation != hosttlb_generation || e->gvpn != gvpn)) {
      Logm("Host TLB slow path");
      hosttlb_read_matrix_slowpath(s, vbase, stride, row, column, msew, transpose, m_name, mreg_id);
    } else {
      Logm("Host TLB fast path");
      uint8_t *host_base = e->offset + vbase;      
#ifdef CONFIG_AME_MEM_ACCESS_CHECK
      svstore_queue_check_matrix_addr_conflict(host_to_guest(host_base), stride,
          row, column, msew, transpose, s->pc, vbase);
#endif
#ifdef CONFIG_DIFFTEST_AMU_CTRL
      amu_ctrl_queue_mls_emplace(mreg_id, 0, transpose, m_name == 'c', m_name == 'a',
        host_to_guest(host_base), stride,
        m_name == 'b' ? column : row,
        m_name == 'b' ? row : column,
        msew
      );
#endif // CONFIG_DIFFTEST_AMU_CTRL
#ifdef CONFIG_SHARE_CTRL
      cutest_mls_emplace(mreg_id, 0, transpose, m_name == 'c', m_name == 'a',
        host_to_guest(host_base), stride,
        m_name == 'b' ? column : row,
        m_name == 'b' ? row : column,
        msew
      );
#endif // CONFIG_SHARE_CTRL
#ifdef CONFIG_TRACE_MATRIX_LOAD_STORE
      fprintf(stderr, "?? fast-path hosttlb_read paddr " FMT_WORD ", len: %d, type: %d\n", (paddr_t)host_to_guest(e->offset) + vbase, 1 << msew, type);
      // guest_to_host(e->offset) when save, so in order to get the original paddr, we use host_to_guest here
#endif // CONFIG_TRACE_MATRIX_LOAD_STORE
#ifndef CONFIG_SHARE_REF
      pmem_read_matrix(host_to_guest(host_base), stride,
                       row, column, msew, transpose, m_name, mreg_id);
#endif // CONFIG_SHARE_REF
    }
  }
}
#endif // CONFIG_RV_AME

extern bool has_two_stage_translation();

#ifdef CONFIG_RVV
void dummy_hosttlb_translate(struct Decode *s, vaddr_t vaddr, int len, bool is_write) {
#ifdef CONFIG_RVH
  if(has_two_stage_translation()){
    // Fast path for guest is not implemented yet
    return;
  }
#endif
  vaddr_t gvpn = hosttlb_vpn(vaddr);

  // Following loc assumes write tlb is right after read tlb by HOSTTLB_SIZE
  HostTLBEntry *used_tlb = &hosttlb[is_write * HOSTTLB_SIZE];
  HostTLBEntry *e = &used_tlb[hosttlb_idx(vaddr)];
  if (unlikely(e->generation != hosttlb_generation || e->gvpn != gvpn)) {
    // TLB miss, fall back to slow path
    return;
  } else {
    // last_access_host_addr is used to indicate TLB hit and fast path is possible
    s->last_access_host_addr = e->offset + vaddr;
    return;
  }
}
#endif // CONFIG_RVV

void hosttlb_write(struct Decode *s, vaddr_t vaddr, int len, word_t data) {
#ifdef CONFIG_RV_ZICFISS
  if (cpu.shadow_stack_access) {
    paddr_t paddr = va2pa(s, vaddr, len, MEM_TYPE_WRITE);
    paddr_write(paddr, len, data, cpu.mode, vaddr);
    return;
  }
#endif
  if (!hosttlb_cacheable(MEM_TYPE_WRITE)) {
    hosttlb_write_slowpath(s, vaddr, len, data);
    return;
  }
#ifdef CONFIG_RVH
  if(has_two_stage_translation()){
    paddr_t paddr = va2pa(s, vaddr, len, MEM_TYPE_WRITE);
    return paddr_write(paddr, len, data, cpu.mode, vaddr);
  }
#endif
  vaddr_t gvpn = hosttlb_vpn(vaddr);
  HostTLBEntry *e = &hostwtlb[hosttlb_idx(vaddr)];
  if (unlikely(e->generation != hosttlb_generation || e->gvpn != gvpn)) {
    hosttlb_write_slowpath(s, vaddr, len, data);
    return;
  }
  uint8_t *host_addr = e->offset + vaddr;
#ifdef CONFIG_PERF_OPT_SHARE
  cpu.pbmt = 0;
  if (dynamic_config.debug_difftest ||
      ISDEF(CONFIG_MEMORY_REGION_ANALYSIS) || ISDEF(CONFIG_AME_MEM_ACCESS_CHECK) ||
      ISDEF(CONFIG_RV_MPT_CHECK) || ISDEF(CONFIG_RV_MBMC)) {
    paddr_write(host_to_guest(host_addr), len, data, cpu.mode, vaddr);
    return;
  }
#endif
#if defined(CONFIG_SHARE) && (defined(CONFIG_STORE_LOG) || defined(CONFIG_STORE_LOG_HASH))
  pmem_record_store_effect(host_to_guest(host_addr), len, data);
#endif
#ifdef CONFIG_DIFFTEST_STORE_COMMIT
  // Also do store commit check with performance optimization enlabled
  store_commit_queue_push(host_to_guest(host_addr), data, len, 0);
#endif // CONFIG_DIFFTEST_STORE_COMMIT
  host_write(host_addr, len, data);
  IFDEF(CONFIG_AME_MEM_ACCESS_CHECK,
      svstore_queue_emplace(host_to_guest(host_addr), len, s->pc, vaddr);)
}

#ifdef CONFIG_RV_AME
void hosttlb_write_matrix(struct Decode *s, vaddr_t vbase, vaddr_t stride,
                          int row, int column, int msew, bool transpose,
                          char m_name, int mreg_id) {
#ifdef CONFIG_RVH
  if (has_two_stage_translation()){
    paddr_t pbase = va2pa(s, vbase, 1 << msew, MEM_TYPE_WRITE);
#ifdef CONFIG_TRACE_MATRIX_LOAD_STORE
    fprintf(stderr, "?? 2-stage hosttlb_write paddr " FMT_WORD ", len: %d, type: %d\n",
      pbase, 1 << msew, MEM_TYPE_MATRIX_WRITE);
#endif // CONFIG_TRACE_MATRIX_LOAD_STORE
    // TODO: print AmuCtrlIO info here
    // fprintf(
    //   "[AmuCtrlIO] op=1 \n"
    //   "            ..."
    //   "\n");
    return paddr_write_matrix(pbase, stride, row, column, msew, transpose,
      cpu.mode, vbase, m_name, mreg_id);
  }
#endif
  vaddr_t gvpn = hosttlb_vpn(vbase);
  HostTLBEntry *e = &hostwtlb[hosttlb_idx(vbase)];
  if (unlikely(e->generation != hosttlb_generation || e->gvpn != gvpn)) {
    hosttlb_write_matrix_slowpath(s, vbase, stride, row, column, msew, transpose, m_name, mreg_id);
    return;
  }
  uint8_t *host_base = e->offset + vbase;
#ifdef CONFIG_DIFFTEST_STORE_COMMIT
  // Also do store commit check with performance optimization enlabled
  matrix_store_commit_queue_push(host_to_guest(host_base), stride, row, column, msew, transpose);
#endif // CONFIG_DIFFTEST_STORE_COMMIT
#ifdef CONFIG_DIFFTEST_AMU_CTRL
  amu_ctrl_queue_mls_emplace(mreg_id, 1, transpose, m_name == 'c', m_name == 'a',
    host_to_guest(host_base), stride,
    m_name == 'b' ? column : row,
    m_name == 'b' ? row : column,
    msew
  );
#endif // CONFIG_DIFFTEST_AMU_CTRL
#ifdef CONFIG_SHARE_CTRL
  cutest_mls_emplace(mreg_id, 1, transpose, m_name == 'c', m_name == 'a',
    host_to_guest(host_base), stride,
    m_name == 'b' ? column : row,
    m_name == 'b' ? row : column,
    msew
  );
#endif // CONFIG_SHARE_CTRL
#ifdef CONFIG_TRACE_MATRIX_LOAD_STORE
  fprintf(stderr, "?? fast-path hosttlb_write paddr " FMT_WORD ", len: %d, type: %d\n",
    (paddr_t)host_to_guest(e->offset) + vbase, 1 << msew, MEM_TYPE_MATRIX_WRITE);
#endif // CONFIG_TRACE_MATRIX_LOAD_STORE
#ifndef CONFIG_SHARE_REF
  pmem_write_matrix(host_to_guest(host_base), stride,
                    row, column, msew, transpose, m_name, mreg_id);
#endif // CONFIG_SHARE_REF
}
#endif // CONFIG_RV_AME
