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
#include <cpu/cpu.h>

#define HOSTTLB_SIZE_SHIFT 12
#define HOSTTLB_SIZE (1 << HOSTTLB_SIZE_SHIFT)

typedef struct {
  uint8_t *offset; // offset from the guest virtual address of the data page to the host virtual address
  vaddr_t gvpn; // guest virtual page number
} HostTLBEntry;

static HostTLBEntry hosttlb[HOSTTLB_SIZE * 3];
static HostTLBEntry* const hostrtlb = &hosttlb[0];
static HostTLBEntry* const hostwtlb = &hosttlb[HOSTTLB_SIZE];
static HostTLBEntry* const hostxtlb = &hosttlb[HOSTTLB_SIZE * 2];

#ifdef CONFIG_RVH
// TLB for translation of guest OSs
static HostTLBEntry hosttlb_g[HOSTTLB_SIZE * 3];
static HostTLBEntry* const hostrtlb_g = &hosttlb_g[0];
static HostTLBEntry* const hostwtlb_g = &hosttlb_g[HOSTTLB_SIZE];
static HostTLBEntry* const hostxtlb_g = &hosttlb_g[HOSTTLB_SIZE * 2];
#endif

static inline vaddr_t hosttlb_vpn(vaddr_t vaddr) {
  return (vaddr >> PAGE_SHIFT);
}

static inline int hosttlb_idx(vaddr_t vaddr) {
  return (hosttlb_vpn(vaddr) % HOSTTLB_SIZE);
}

void hosttlb_flush(vaddr_t vaddr) {
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

uint8_t *hosttlb_lookup(vaddr_t vaddr, int type) {
  Logm("hosttlb_reading " FMT_WORD, vaddr);
  vaddr_t gvpn = hosttlb_vpn(vaddr);
  HostTLBEntry *e = (type == MEM_TYPE_IFETCH) ?  &hostxtlb[hosttlb_idx(vaddr)] :  
    (type == MEM_TYPE_READ) ? &hostrtlb[hosttlb_idx(vaddr)] : &hostrtlb[hosttlb_idx(vaddr)];
  if (unlikely(e->gvpn != gvpn)) {
    Logm("Host TLB misses");
    return (uint8_t *)MEM_RET_FAIL;
  } else {
    Logm("Host TLB hits");
    return e->offset + vaddr;
  }
}

void hosttlb_insert(vaddr_t vaddr, paddr_t paddr, int type) {
  Logm("hosttlb_writing " FMT_WORD, paddr);
  HostTLBEntry *e = (type == MEM_TYPE_IFETCH) ?  &hostxtlb[hosttlb_idx(vaddr)] :  
    (type == MEM_TYPE_READ) ? &hostrtlb[hosttlb_idx(vaddr)] : &hostrtlb[hosttlb_idx(vaddr)];

  e->offset = guest_to_host(paddr) - vaddr;
  e->gvpn = hosttlb_vpn(vaddr);
}

#ifdef CONFIG_RVH

void hosttlb_guest_flush(paddr_t gpaddr) {
  if (gpaddr == 0) {
    memset(hosttlb, -1, sizeof(hosttlb));
  } else {
    vaddr_t gvpn = hosttlb_vpn(gpaddr);
    int idx = hosttlb_idx(gpaddr);
    if (hostrtlb_g[idx].gvpn == gvpn) hostrtlb_g[idx].gvpn = (sword_t)-1;
    if (hostwtlb_g[idx].gvpn == gvpn) hostwtlb_g[idx].gvpn = (sword_t)-1;
    if (hostxtlb_g[idx].gvpn == gvpn) hostxtlb_g[idx].gvpn = (sword_t)-1;
  }
}

uint8_t *hosttlb_guest_lookup(paddr_t gpaddr, int type) {
  Logm("hosttlb_reading " FMT_WORD, gpaddr);
  paddr_t gvpn = hosttlb_vpn(gpaddr);
  HostTLBEntry *e = type == MEM_TYPE_IFETCH ?  &hostxtlb_g[hosttlb_idx(gpaddr)] :  
    (type == MEM_TYPE_READ) ? &hostrtlb_g[hosttlb_idx(gpaddr)] : &hostrtlb_g[hosttlb_idx(gpaddr)];
  if (unlikely(e->gvpn != gvpn)) {
    Logm("Host Guest TLB misses");
    return (uint8_t *)MEM_RET_FAIL;
  } else {
    Logm("Host Guest TLB hits");
    return e->offset + gpaddr;
  }
}

void hosttlb_guest_insert(paddr_t gpaddr, paddr_t paddr, int type) {
  Logm("hosttlb_writing " FMT_WORD, gpaddr);
  HostTLBEntry *e = type == MEM_TYPE_IFETCH ?  &hostxtlb_g[hosttlb_idx(gpaddr)] :  
    (type == MEM_TYPE_READ) ? &hostrtlb_g[hosttlb_idx(gpaddr)] : &hostrtlb_g[hosttlb_idx(gpaddr)];

  e->offset = guest_to_host(paddr) - gpaddr;
  e->gvpn = hosttlb_vpn(gpaddr);
}

#endif // CONFIG_RVH

void hosttlb_init() {
  hosttlb_flush(0);
#ifdef CONFIG_RVH
  hosttlb_guest_flush(0);
#endif
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
  word_t data = paddr_read(paddr, len, type, cpu.mode, vaddr);
  if (likely(in_pmem(paddr))) {
    hosttlb_insert(vaddr, paddr, type);
  }
  Logtr("Slowpath, vaddr " FMT_WORD " --> paddr: " FMT_PADDR, vaddr, paddr);
  return data;
}

__attribute__((noinline))
static void hosttlb_write_slowpath(struct Decode *s, vaddr_t vaddr, int len, word_t data) {
  paddr_t paddr = va2pa(s, vaddr, len, MEM_TYPE_WRITE);
  paddr_write(paddr, len, data, cpu.mode, vaddr);
  if (likely(in_pmem(paddr))) {
    hosttlb_insert(vaddr, paddr, MEM_TYPE_WRITE);
  }
}

word_t hosttlb_read(struct Decode *s, vaddr_t vaddr, int len, int type) {
  Logm("hosttlb_reading " FMT_WORD, vaddr);
#ifdef CONFIG_RVH
  extern bool has_two_stage_translation();
  if (has_two_stage_translation()) {
    paddr_t paddr = va2pa(s, vaddr, len, type);
    return paddr_read(paddr, len, type, cpu.mode, vaddr);
  }
#endif

  uint8_t *paddr = hosttlb_lookup(vaddr, type);
  if ((paddr_t)paddr == MEM_RET_FAIL) {
    Logm("Host TLB slow path");
    return hosttlb_read_slowpath(s, vaddr, len, type);
  } else {
    Logm("Host TLB fast path");
    return host_read(paddr, len);
  }
}

void hosttlb_write(struct Decode *s, vaddr_t vaddr, int len, word_t data) {
#ifdef CONFIG_RVH
  extern bool has_two_stage_translation();
  if(has_two_stage_translation()){
    paddr_t paddr = va2pa(s, vaddr, len, MEM_TYPE_WRITE);
    return paddr_write(paddr, len, data, cpu.mode, vaddr);
  }
#endif

  uint8_t *paddr = hosttlb_lookup(vaddr, MEM_TYPE_WRITE);
  if ((paddr_t)paddr == MEM_RET_FAIL) {
    hosttlb_write_slowpath(s, vaddr, len, data);
  } else {
    host_write(paddr, len, data);
  }
}
