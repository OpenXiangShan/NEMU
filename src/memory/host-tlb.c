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
#include <memory/host-tlb.h>
#include <memory/sparseram.h>
#include <cpu/cpu.h>
#include <cpu/decode.h>


#define HOSTTLB_SIZE_SHIFT 12

#define HOSTTLB_SIZE (1 << HOSTTLB_SIZE_SHIFT)

/*
  Host tlb for address translation from (guest) virtual address to physical address
  Wraning: Reusage of host tlb for both VA->PA and GVA->GPA may lead to unpredictable fault 
    if we don't flush it when VM Exit or dynamically and transparently change address mapping in application.
*/

typedef struct {
  uint8_t *offset; // offset from the virtual address of the data page to the host virtual address

  uint64_t tag[0];
#ifdef CONFIG_RVH
  struct {
    // virtual page number, that's fine to use only 63 bits.
    vaddr_t vpn: 63; 
    // Denote whether the entry is inserted under virtualization mode or normal mode, which can aslo be effected by hlvx or mstatus.ppv
    bool virt: 1; 
  };
#else
  vaddr_t vpn;
#endif

} HostTLBEntry;

static bool hosttlb_dirt; // Dirt flag for accelerating program using onlyStage2 translation
static HostTLBEntry hosttlb[HOSTTLB_SIZE * 3];
static HostTLBEntry* const hostrtlb = &hosttlb[0];
static HostTLBEntry* const hostwtlb = &hosttlb[HOSTTLB_SIZE];
static HostTLBEntry* const hostxtlb = &hosttlb[HOSTTLB_SIZE * 2];

#ifdef CONFIG_RVH 

/*
  Host tlb for address translation of G-stage, which help translate guest physical address to host physical address
*/

typedef struct {
  // offset from the guest virtual address of the data page to the host virtual address
  paddr_t offset; 
  // guest physical page number
  paddr_t gppn; 
} HostGTLBEntry;

static HostGTLBEntry hostgtlb[HOSTTLB_SIZE * 3];
static HostGTLBEntry* const hostgrtlb = &hostgtlb[0];
static HostGTLBEntry* const hostgwtlb = &hostgtlb[HOSTTLB_SIZE];
static HostGTLBEntry* const hostgxtlb = &hostgtlb[HOSTTLB_SIZE * 2];

#endif

extern bool has_two_stage_translation();

#define HOSTTLB_ADDR2PN(addr) (addr >> PAGE_SHIFT)

#define HOSTTLB_ADDR2IDX(addr) (HOSTTLB_ADDR2PN(addr) % HOSTTLB_SIZE)


static inline paddr_t va2pa(struct Decode *s, vaddr_t vaddr, int len, int type) {
  if (type != MEM_TYPE_IFETCH) save_globals(s);
  // int ret = isa_mmu_check(vaddr, len, type);
  // if (ret == MMU_DIRECT) return vaddr;
  paddr_t pg_base = isa_mmu_translate(vaddr, len, type);
  int ret = pg_base & PAGE_MASK;
  assert(ret == MEM_RET_OK);
  return pg_base | (vaddr & PAGE_MASK);
}


uint8_t *hosttlb_lookup(vaddr_t vaddr, int type) {
  int id = HOSTTLB_ADDR2IDX(vaddr);
  const HostTLBEntry *e = (type == MEM_TYPE_IFETCH) ?  &hostxtlb[id] : 
    (type == MEM_TYPE_READ) ? &hostrtlb[id] : &hostwtlb[id];

  if (e->vpn == HOSTTLB_ADDR2PN(vaddr) 
      && MUXDEF(CONFIG_RVH, e->virt == has_two_stage_translation(), true)) {
    return e->offset + vaddr; 
  }
  return HOSTTLB_PTR_FAIL_RET;
}


void hosttlb_insert(vaddr_t vaddr, paddr_t paddr, int type) {
  int id = HOSTTLB_ADDR2IDX(vaddr);
  HostTLBEntry *e = (type == MEM_TYPE_IFETCH) ?  &hostxtlb[id] : 
    (type == MEM_TYPE_READ) ? &hostrtlb[id] : &hostwtlb[id];

  e->offset = MUXDEF(CONFIG_USE_SPARSEMM, (uint8_t *)(paddr - vaddr), guest_to_host(paddr) - vaddr);
  e->vpn = HOSTTLB_ADDR2PN(vaddr);
  IFDEF(CONFIG_RVH, e->virt = has_two_stage_translation());

  hosttlb_dirt = true;
}


void hosttlb_flush(vaddr_t vaddr) {
  if (vaddr == 0) {
    if (hosttlb_dirt) {
      hosttlb_dirt = false;
      memset(hosttlb, -1, sizeof(hosttlb));
    }
  } else {
    vaddr_t vpn = HOSTTLB_ADDR2PN(vaddr);
    int idx = HOSTTLB_ADDR2IDX(vaddr);
    if (hostrtlb[idx].vpn == vpn) *(hostrtlb[idx].tag) = -1;
    if (hostwtlb[idx].vpn == vpn) *(hostwtlb[idx].tag) = -1;
    if (hostxtlb[idx].vpn == vpn) *(hostxtlb[idx].tag) = -1;
  }
}


#ifdef CONFIG_RVH 

paddr_t hostgtlb_lookup(paddr_t gpaddr, int type) {
  int id = HOSTTLB_ADDR2IDX(gpaddr);
  const HostGTLBEntry *e = (type == MEM_TYPE_IFETCH) ?  &hostgxtlb[id] : 
    (type == MEM_TYPE_READ) ? &hostgrtlb[id] : &hostgwtlb[id];

  if (e->gppn == HOSTTLB_ADDR2PN(gpaddr)) {
    return e->offset + gpaddr; 
  }
  return HOSTTLB_PADDR_FAIL_RET;
}


void hostgtlb_insert(paddr_t gpaddr, paddr_t paddr, int type) {
  int id = HOSTTLB_ADDR2IDX(gpaddr);
  HostGTLBEntry *e = (type == MEM_TYPE_IFETCH) ?  &hostgxtlb[id] : 
    (type == MEM_TYPE_READ) ? &hostgrtlb[id] : &hostgwtlb[id];

  // Unlike hosttlb_insert, in order to keep the translation uniform, 
  // we do not translate the paddr to physical address of NEMU.
  e->offset = paddr - gpaddr;
  e->gppn = HOSTTLB_ADDR2PN(gpaddr);
}


void hostgtlb_flush(paddr_t gpaddr) {
  if (gpaddr == 0) {
    memset(hostgtlb, -1, sizeof(hostgtlb));
  } else {
    paddr_t gppn = HOSTTLB_ADDR2PN(gpaddr);
    int idx = HOSTTLB_ADDR2IDX(gpaddr);
    if (hostgrtlb[idx].gppn == gppn) hostgrtlb[idx].gppn = (sword_t)-1;
    if (hostgwtlb[idx].gppn == gppn) hostgwtlb[idx].gppn = (sword_t)-1;
    if (hostgxtlb[idx].gppn == gppn) hostgxtlb[idx].gppn = (sword_t)-1;
  }

  /*
    When changing mapping from GPA to PA, it is quite embarrassed that the corresponding mapping from GVA to PA become invalid.
    It is both wasteful to find the entry corresponding to PA and to flush the whole TLB. Maybe the last one will be faster as SIMD exists.
  */ 

  hosttlb_flush(0);
}

#endif // CONFIG_RVH

void hosttlb_init() {
  hosttlb_dirt = true;
  hosttlb_flush(0);
#ifdef CONFIG_RVH
  hostgtlb_flush(0);
#endif // CONFIG_RVH
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

  uint8_t *dst_ptr = hosttlb_lookup(vaddr, type);
  if (unlikely(dst_ptr == HOSTTLB_PTR_FAIL_RET)) {
    Logm("Host TLB slow path");
    return hosttlb_read_slowpath(s, vaddr, len, type);
  } else {
    Logm("Host TLB fast path");
#ifdef CONFIG_USE_SPARSEMM
    return sparse_mem_wread(get_sparsemm(), (vaddr_t)dst_ptr, len);
#else
    return host_read(dst_ptr, len);
#endif
  }
}


void hosttlb_write(struct Decode *s, vaddr_t vaddr, int len, word_t data) {
  Logm("hosttlb_writing " FMT_WORD, vaddr);

  uint8_t *dst_ptr = hosttlb_lookup(vaddr, MEM_TYPE_WRITE);
  if (unlikely(dst_ptr == HOSTTLB_PTR_FAIL_RET)) {
    Logm("Host TLB slow path");
    hosttlb_write_slowpath(s, vaddr, len, data);
    return;
  } else {
    Logm("Host TLB fast path");
#ifdef CONFIG_USE_SPARSEMM
    sparse_mem_write(get_sparsemm(), (vaddr_t)dst_ptr, len, data);
#else
    host_write(dst_ptr, len, data);
#endif
  }
}
