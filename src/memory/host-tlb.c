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
#include <memory/vaddr.h>
#include <memory/paddr.h>
#include <memory/sparseram.h>
#include <cpu/cpu.h>
#include <cpu/decode.h>

#define HOSTTLB_SIZE_SHIFT 12
#define HOSTTLB_SIZE (1 << HOSTTLB_SIZE_SHIFT)

typedef struct {
  uint8_t *offset; // offset from the guest virtual address of the data page to the host virtual address
  vaddr_t gvpn; // guest virtual page number
} HostTLBEntry;

static HostTLBEntry hosttlb[HOSTTLB_SIZE * 3];
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
  if (vaddr == 0) {
    memset(hosttlb, -1, sizeof(hosttlb));
  } else {
    vaddr_t gvpn = hosttlb_vpn(vaddr);
    int idx = hosttlb_idx(vaddr);
    if (hostrtlb[idx].gvpn == gvpn) hostrtlb[idx].gvpn = (sword_t)-1;
    if (hostwtlb[idx].gvpn == gvpn) hostwtlb[idx].gvpn = (sword_t)-1;
    if (hostxtlb[idx].gvpn == gvpn) hostxtlb[idx].gvpn = (sword_t)-1;
  }
}

void hosttlb_init() {
  hosttlb_flush(0);
}

static paddr_t va2pa(struct Decode *s, vaddr_t vaddr, int len, int type) {
  if (type != MEM_TYPE_IFETCH) save_globals(s);
  // int ret = isa_mmu_check(vaddr, len, type);
  // if (ret == MMU_DIRECT) return vaddr;
  paddr_t pg_base = isa_mmu_translate(vaddr, len, type);
  int ret = pg_base & PAGE_MASK;
  assert(ret == MEM_RET_OK);
  return pg_base | (vaddr & PAGE_MASK);
}

__attribute__((noinline))
static word_t hosttlb_read_slowpath(struct Decode *s, vaddr_t vaddr, int len, int type) {
  paddr_t paddr = va2pa(s, vaddr, len, type);
  word_t data = paddr_read(paddr, len, type, type, cpu.mode, vaddr);
  if (
    MUXDEF(CONFIG_RV_MBMC, isa_bmc_check_permission(paddr, len, 0, 0), true) &&
    likely(in_pmem(paddr))
  ) {
    HostTLBEntry *e = type == MEM_TYPE_IFETCH ?
      &hostxtlb[hosttlb_idx(vaddr)] : &hostrtlb[hosttlb_idx(vaddr)];
    #ifdef CONFIG_USE_SPARSEMM
    e->offset = (uint8_t *)(paddr - vaddr);
    #else
    e->offset = guest_to_host(paddr) - vaddr;
    #endif
    e->gvpn = hosttlb_vpn(vaddr);
  }
  Logtr("Slowpath, vaddr " FMT_WORD " --> paddr: " FMT_PADDR, vaddr, paddr);
  return data;
}

__attribute__((noinline))
static void hosttlb_write_slowpath(struct Decode *s, vaddr_t vaddr, int len, word_t data) {
  paddr_t paddr = va2pa(s, vaddr, len, MEM_TYPE_WRITE);
  paddr_write(paddr, len, data, cpu.mode, vaddr);
  if (
    MUXDEF(CONFIG_RV_MBMC, isa_bmc_check_permission(paddr, len, 0, 0), true) &&
    likely(in_pmem(paddr))
  ) {
    HostTLBEntry *e = &hostwtlb[hosttlb_idx(vaddr)];
    #ifdef CONFIG_USE_SPARSEMM
    e->offset = (uint8_t *)(paddr - vaddr);
    #else
    e->offset = guest_to_host(paddr) - vaddr;
    #endif
    e->gvpn = hosttlb_vpn(vaddr);
  }
}

word_t hosttlb_read(struct Decode *s, vaddr_t vaddr, int len, int type) {
  Logm("hosttlb_reading " FMT_WORD, vaddr);
#ifdef CONFIG_RVH
  extern bool has_two_stage_translation();
  if(has_two_stage_translation()){
    paddr_t paddr = va2pa(s, vaddr, len, type);
    return paddr_read(paddr, len, type, type, cpu.mode, vaddr);
  }
#endif
  vaddr_t gvpn = hosttlb_vpn(vaddr);
  HostTLBEntry *e = type == MEM_TYPE_IFETCH ?
    &hostxtlb[hosttlb_idx(vaddr)] : &hostrtlb[hosttlb_idx(vaddr)];
  if (unlikely(e->gvpn != gvpn)) {
    Logm("Host TLB slow path");
    return hosttlb_read_slowpath(s, vaddr, len, type);
  } else {
    Logm("Host TLB fast path");
    #ifdef CONFIG_USE_SPARSEMM
    return sparse_mem_wread(get_sparsemm(), (vaddr_t)e->offset + vaddr, len);
    #else
    return host_read(e->offset + vaddr, len);
    #endif
  }
}
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
  if (unlikely(e->gvpn != gvpn)) {
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
#ifdef CONFIG_RVH
  if(has_two_stage_translation()){
    paddr_t paddr = va2pa(s, vaddr, len, MEM_TYPE_WRITE);
    return paddr_write(paddr, len, data, cpu.mode, vaddr);
  }
#endif
  vaddr_t gvpn = hosttlb_vpn(vaddr);
  HostTLBEntry *e = &hostwtlb[hosttlb_idx(vaddr)];
  if (unlikely(e->gvpn != gvpn)) {
    hosttlb_write_slowpath(s, vaddr, len, data);
    return;
  }
#ifdef CONFIG_USE_SPARSEMM
  sparse_mem_wwrite(get_sparsemm(), (vaddr_t)e->offset + vaddr, len, data);
#else // NOT CONFIG_USE_SPARSEMM
  uint8_t *host_addr = e->offset + vaddr;
#ifdef CONFIG_DIFFTEST_STORE_COMMIT
  // Also do store commit check with performance optimization enlabled
  store_commit_queue_push(host_to_guest(host_addr), data, len, 0);
#endif // CONFIG_DIFFTEST_STORE_COMMIT
  host_write(host_addr, len, data);
#endif // NOT CONFIG_USE_SPARSEMM
}
