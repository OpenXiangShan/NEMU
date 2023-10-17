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
#include <cpu/cpu.h>

#define HOSTTLB_SIZE_SHIFT 12
#define HOSTTLB_SIZE (1 << HOSTTLB_SIZE_SHIFT)

typedef struct {
  uint8_t *offset; // offset from the virtual address of the data page to the host virtual address
  vaddr_t vpn; // virtual page number
} HostTLBEntry;

static HostTLBEntry hosttlb[HOSTTLB_SIZE * 3];
static HostTLBEntry* const hostrtlb = &hosttlb[0];
static HostTLBEntry* const hostwtlb = &hosttlb[HOSTTLB_SIZE];
static HostTLBEntry* const hostxtlb = &hosttlb[HOSTTLB_SIZE * 2];

#ifdef CONFIG_RVH 

typedef struct {
  paddr_t offset; // offset from the guest virtual address of the data page to the host virtual address
  paddr_t gppn; // guest physical page number
} HostGTLBEntry;

static HostGTLBEntry hostgtlb[HOSTTLB_SIZE * 3];
static HostGTLBEntry* const hostgrtlb = &hostgtlb[0];
static HostGTLBEntry* const hostgwtlb = &hostgtlb[HOSTTLB_SIZE];
static HostGTLBEntry* const hostgxtlb = &hostgtlb[HOSTTLB_SIZE * 2];

typedef struct {
  paddr_t gppn; // guest physical address
  vaddr_t gvpn; // guest virtual page number
} HostVSTLBEntry;

static HostVSTLBEntry hostvstlb[HOSTTLB_SIZE * 3];
static HostVSTLBEntry* const hostvsrtlb = &hostvstlb[0];
static HostVSTLBEntry* const hostvswtlb = &hostvstlb[HOSTTLB_SIZE];
static HostVSTLBEntry* const hostvsxtlb = &hostvstlb[HOSTTLB_SIZE * 2];

#endif


static inline vaddr_t hosttlb_vpn(vaddr_t vaddr) {
  return (vaddr >> PAGE_SHIFT);
}

static inline int hosttlb_idx(vaddr_t vaddr) {
  return (hosttlb_vpn(vaddr) % HOSTTLB_SIZE);
}


#ifdef CONFIG_RVH

static inline paddr_t hostgtlb_ppn(paddr_t gpaddr) {
  return (gpaddr >> PAGE_SHIFT);
}

static inline int hostgtlb_idx(paddr_t gpaddr) {
  return (hostgtlb_ppn(gpaddr) % HOSTTLB_SIZE);
}

static inline vaddr_t hostvstlb_vpn(vaddr_t gvaddr) {
  return (gvaddr >> PAGE_SHIFT);
}

static inline int hostvstlb_idx(vaddr_t gvaddr) {
  return (hostvstlb_vpn(gvaddr) % HOSTTLB_SIZE);
}

extern bool has_two_stage_translation();

#endif // CONFIG_RVH


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
  int id = hosttlb_idx(vaddr);
  const HostTLBEntry *e = (type == MEM_TYPE_IFETCH) ?  &hostxtlb[id] : 
    (type == MEM_TYPE_READ) ? &hostrtlb[id] : &hostwtlb[id];

  if (e->vpn == hosttlb_vpn(vaddr)) {
    return e->offset + vaddr; 
  }
  return HOSTTLB_PTR_FAIL_RET;
}

void hosttlb_insert(vaddr_t vaddr, paddr_t paddr, int type) {
  int id = hosttlb_idx(vaddr);
  HostTLBEntry *e = (type == MEM_TYPE_IFETCH) ?  &hostxtlb[id] : 
    (type == MEM_TYPE_READ) ? &hostrtlb[id] : &hostwtlb[id];

  e->offset = guest_to_host(paddr) - vaddr;
  e->vpn = hosttlb_vpn(vaddr);
}

void hosttlb_flush(vaddr_t vaddr) {
  if (vaddr == 0) {
    memset(hosttlb, -1, sizeof(hosttlb));
  } else {
    vaddr_t vpn = hosttlb_vpn(vaddr);
    int idx = hosttlb_idx(vaddr);
    if (hostrtlb[idx].vpn == vpn) hostrtlb[idx].vpn = (sword_t)-1;
    if (hostwtlb[idx].vpn == vpn) hostwtlb[idx].vpn = (sword_t)-1;
    if (hostxtlb[idx].vpn == vpn) hostxtlb[idx].vpn = (sword_t)-1;
  }
}


#ifdef CONFIG_RVH 

static inline paddr_t gpa2pa(paddr_t gpaddr, vaddr_t vaddr, int len, int type) {
  paddr_t pg_base = isa_mmu_translate_only_stage2(gpaddr, vaddr, len, type);
  int ret = pg_base & PAGE_MASK;
  assert(ret == MEM_RET_OK);
  return pg_base | (gpaddr & PAGE_MASK);
}

paddr_t hostgtlb_lookup(paddr_t gpaddr, int type) {
  int id = hostgtlb_idx(gpaddr);
  const HostGTLBEntry *e = (type == MEM_TYPE_IFETCH) ?  &hostgxtlb[id] : 
    (type == MEM_TYPE_READ) ? &hostgrtlb[id] : &hostgwtlb[id];

  if (e->gppn == hostgtlb_ppn(gpaddr)) {
    return e->offset + gpaddr; 
  }
  return HOSTTLB_PADDR_FAIL_RET;
}

void hostgtlb_insert(paddr_t gpaddr, paddr_t paddr, int type) {
  int id = hostgtlb_idx(gpaddr);
  HostGTLBEntry *e = (type == MEM_TYPE_IFETCH) ?  &hostgxtlb[id] : 
    (type == MEM_TYPE_READ) ? &hostgrtlb[id] : &hostgwtlb[id];

  // Unlike hosttlb_insert, in order to keep the translation uniform, 
  // we do not translate the paddr to physical address of NEMU.
  e->offset = paddr - gpaddr;
  e->gppn = hostgtlb_ppn(gpaddr);
}

void hostgtlb_flush(paddr_t gpaddr) {
  if (gpaddr == 0) {
    memset(hostgtlb, -1, sizeof(hostgtlb));
  } else {
    paddr_t gppn = hostgtlb_ppn(gpaddr);
    int idx = hostgtlb_idx(gpaddr);
    if (hostgrtlb[idx].gppn == gppn) hostgrtlb[idx].gppn = (sword_t)-1;
    if (hostgwtlb[idx].gppn == gppn) hostgwtlb[idx].gppn = (sword_t)-1;
    if (hostgxtlb[idx].gppn == gppn) hostgxtlb[idx].gppn = (sword_t)-1;
  }
}

paddr_t hostvstlb_lookup(vaddr_t gvaddr, int type) {
  int id = hostvstlb_idx(gvaddr);
  const HostVSTLBEntry *e = (type == MEM_TYPE_IFETCH) ?  &hostvsxtlb[id] : 
    (type == MEM_TYPE_READ) ? &hostvsrtlb[id] : &hostvswtlb[id];
  if (e->gvpn == hostvstlb_vpn(gvaddr)) {
    return e->gppn; 
  }
  return HOSTTLB_PADDR_FAIL_RET;
}

void hostvstlb_insert(vaddr_t gvaddr, paddr_t gpaddr, int type) {
  int id = hostvstlb_idx(gvaddr);
  HostVSTLBEntry *e = (type == MEM_TYPE_IFETCH) ?  &hostvsxtlb[id] : 
    (type == MEM_TYPE_READ) ? &hostvsrtlb[id] : &hostvswtlb[id];

  // Unlike hosttlb_insert, in order to keep the translation uniform, 
  // we do not translate the paddr to physical address of NEMU.
  e->gppn = gpaddr;
  e->gvpn = hostvstlb_vpn(gvaddr);
}

void hostvstlb_flush(vaddr_t gvaddr) {
  if (gvaddr == 0) {
    memset(hostvstlb, -1, sizeof(hostvstlb));
  } else {
    paddr_t gvpn = hostvstlb_vpn(gvaddr);
    int idx = hostvstlb_idx(gvaddr);
    if (hostvsrtlb[idx].gvpn == gvpn) hostvsrtlb[idx].gvpn = (sword_t)-1;
    if (hostvswtlb[idx].gvpn == gvpn) hostvswtlb[idx].gvpn = (sword_t)-1;
    if (hostvsxtlb[idx].gvpn == gvpn) hostvsxtlb[idx].gvpn = (sword_t)-1;
  }
}

#endif // CONFIG_RVH

void hosttlb_init() {
  hosttlb_flush(0);
#ifdef CONFIG_RVH
  hostgtlb_flush(0);
  hostvstlb_flush(0);
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

#ifdef CONFIG_RVH
  if(has_two_stage_translation()){
    paddr_t gpaddr = hostvstlb_lookup(vaddr, type);

    paddr_t paddr;
    if (unlikely(gpaddr == HOSTTLB_PADDR_FAIL_RET)) {
      paddr = va2pa(s, vaddr, len, type);
    } else {
      paddr = gpa2pa(gpaddr, vaddr, len, type);
    }
    return paddr_read(paddr, len, type, cpu.mode, vaddr);
  }
#endif

  uint8_t *dst_ptr = hosttlb_lookup(vaddr, type);
  if (unlikely(dst_ptr == HOSTTLB_PTR_FAIL_RET)) {
    Logm("Host TLB slow path");
    return hosttlb_read_slowpath(s, vaddr, len, type);
  } else {
    Logm("Host TLB fast path");
    return host_read(dst_ptr, len);
  }
}

void hosttlb_write(struct Decode *s, vaddr_t vaddr, int len, word_t data) {
  Logm("hosttlb_writing " FMT_WORD, vaddr);

#ifdef CONFIG_RVH
  if (has_two_stage_translation()) { 
    paddr_t gpaddr = hostvstlb_lookup(vaddr, MEM_TYPE_WRITE);
    paddr_t paddr;
    if (gpaddr == HOSTTLB_PADDR_FAIL_RET) {
      paddr = va2pa(s, vaddr, len, MEM_TYPE_WRITE);
    } else {
      paddr = gpa2pa(gpaddr, vaddr, len, MEM_TYPE_WRITE);
    }
    paddr_write(paddr, len, data, cpu.mode, vaddr);
    return;
  }
#endif

  uint8_t *dst_ptr = hosttlb_lookup(vaddr, MEM_TYPE_WRITE);
  if (unlikely(dst_ptr == HOSTTLB_PTR_FAIL_RET)) {
    Logm("Host TLB slow path");
    hosttlb_write_slowpath(s, vaddr, len, data);
  } else {
    Logm("Host TLB fast path");
    host_write(dst_ptr, len, data);
  }
}
