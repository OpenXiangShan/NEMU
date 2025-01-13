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
#include <memory/store_queue_wrapper.h>
#include <memory/sparseram.h>
#include <device/mmio.h>
#include <stdlib.h>
#include <time.h>
#include <cpu/cpu.h>
#include "../local-include/csr.h"
#include "../local-include/intr.h"

bool is_in_mmio(paddr_t addr);

unsigned long MEMORY_SIZE = CONFIG_MSIZE;
unsigned int PMEM_HARTID = 0;

extern Decode *prev_s;

#ifdef CONFIG_LIGHTQS
#define PMEMBASE 0x1100000000ul
#elif CONFIG_ENABLE_IDEAL_MODEL
#define PMEMBASE 0x3000000000ul
#else
#define PMEMBASE 0x100000000ul
#endif // CONFIG_LIGHTQS

#ifdef CONFIG_USE_MMAP
#include <sys/mman.h>
#include <fcntl.h>
#include <unistd.h>
static uint8_t *pmem = NULL;
#elif CONFIG_ENABLE_MEM_DEDUP
// When memory deduplication is enabled, the pmem is allocated by DUT
static uint8_t *pmem = NULL;
#else
static uint8_t pmem[CONFIG_MSIZE] PG_ALIGN = {};
#endif

#ifdef CONFIG_USE_SPARSEMM
void* sparse_mm = NULL;
#endif

#ifdef CONFIG_STORE_LOG
struct store_log {
#ifdef CONFIG_LIGHTQS
  uint64_t inst_cnt;
#endif // CONFIG_LIGHTQS
  paddr_t addr;
  word_t orig_data;
  // new value and write length makes no sense for restore
} store_log_buf[CONFIG_STORE_LOG_SIZE];

uint64_t store_log_ptr = 0;
#ifdef CONFIG_LIGHTQS
struct store_log spec_store_log_buf[CONFIG_STORE_LOG_SIZE];
uint64_t spec_store_log_ptr = CONFIG_SPEC_GAP;
#endif // CONFIG_LIGHTQS
#endif // CONFIG_STORE_LOG

#define HOST_PMEM_OFFSET (uint8_t *)(pmem - CONFIG_MBASE)

uint8_t *get_pmem()
{
  return pmem;
}

char *mapped_cpt_file = NULL;
bool map_image_as_output_cpt = false;

#ifdef CONFIG_USE_SPARSEMM
void * get_sparsemm(){
  return sparse_mm;
}
#endif

uint8_t* guest_to_host(paddr_t paddr) { return paddr + HOST_PMEM_OFFSET; }
paddr_t host_to_guest(uint8_t *haddr) { return haddr - HOST_PMEM_OFFSET; }

static inline word_t pmem_read(paddr_t addr, int len) {
#ifdef CONFIG_MEMORY_REGION_ANALYSIS
  analysis_memory_commit(addr);
#endif
  #ifdef CONFIG_USE_SPARSEMM
  return sparse_mem_wread(sparse_mm, addr, len);
  #else
  return host_read(guest_to_host(addr), len);
  #endif
}

static inline void pmem_write(paddr_t addr, int len, word_t data, int cross_page_store) {
#ifdef CONFIG_ENABLE_IDEAL_MODEL
  extern void mark_memtrace_to_fm(paddr_t addr, int len, word_t data, int cross_page_store);
  word_t old_data = pmem_read(addr, len);
  mark_memtrace_to_fm(addr, len, old_data, cross_page_store);
#endif 
#ifdef CONFIG_DIFFTEST_STORE_COMMIT
  store_commit_queue_push(addr, data, len, cross_page_store);
#endif
#ifdef CONFIG_MEMORY_REGION_ANALYSIS
  analysis_memory_commit(addr);
#endif
  #ifdef CONFIG_USE_SPARSEMM
  sparse_mem_wwrite(sparse_mm, addr, len, data);
  #else
  switch (len) {
    case 1: case 2: case 4:
    IFDEF(CONFIG_ISA64, case 8:)
      host_write(guest_to_host(addr), len, data); break;
    // strange length, only valid from cross page write
    case 3:
    IFDEF(CONFIG_ISA64, case 5: case 6: case 7:)
      if (cross_page_store) {
        int i = 0;
        for (; i < len; i++, addr++) {
          host_write(guest_to_host(addr), 1, data & 0xffUL);
          data >>= 8;
        }
        break;
      }
    IFDEF(CONFIG_RT_CHECK, default: assert(0));
  }
  #endif
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
  if (type == MEM_TYPE_IFETCH || type == MEM_TYPE_IFETCH_READ) {
    cause = EX_IAF;
  } else if (cpu.amo || type == MEM_TYPE_WRITE || type == MEM_TYPE_WRITE_READ) {
    cause = EX_SAF; 
  }
  raise_access_fault(cause, vaddr);
}

static inline void isa_mmio_misalign_data_addr_check(paddr_t paddr, vaddr_t vaddr, int len, int type, int is_cross_page) {
  if (unlikely((paddr & (len - 1)) != 0) || is_cross_page) {
    Logm("addr misaligned happened: paddr:" FMT_PADDR " vaddr:" FMT_WORD " len:%d type:%d pc:%lx", paddr, vaddr, len, type, cpu.pc);
    if (ISDEF(CONFIG_MMIO_AC_SOFT)) {
      int ex = cpu.amo || type == MEM_TYPE_WRITE ? EX_SAM : EX_LAM;
      INTR_TVAL_REG(ex) = vaddr;
      longjmp_exception(ex);
    }
  }
}

void allocate_memory_with_mmap()
{
#ifdef CONFIG_USE_MMAP
#ifdef CONFIG_USE_SPARSEMM
  sparse_mm = sparse_mem_new(4, 1024); //4kB
#else
  // Note: we are using MAP_FIXED here, in the SHARED mode, even if
  // init_mem may be called multiple times, the memory space will be
  // allocated only once at the first time called.
  // See https://man7.org/linux/man-pages/man2/mmap.2.html for details.
  // config ideal model in this
  void *pmem_base = (void *)(PMEMBASE + PMEM_HARTID * MEMORY_SIZE);
#ifdef CONFIG_ENABLE_IDEAL_MODEL
  // kernel version >= 4.17
  // use MAP_FIXED_NOREPLACE, because we dont want ideal model
  // demage the vm-space already available to the process. 
  void *ret = mmap(pmem_base, MEMORY_SIZE, PROT_READ | PROT_WRITE,
      MAP_ANONYMOUS | MAP_PRIVATE | MAP_FIXED_NOREPLACE | MAP_NORESERVE, -1, 0);
#else
  void *ret = mmap(pmem_base, MEMORY_SIZE, PROT_READ | PROT_WRITE,
      MAP_ANONYMOUS | MAP_PRIVATE | MAP_FIXED | MAP_NORESERVE, -1, 0);
#endif
  if (ret != pmem_base) {
    perror("mmap");
    assert(0);
  }
  pmem = (uint8_t*)ret;
#endif
#endif // CONFIG_USE_MMAP
}

void init_mem() {
  allocate_memory_with_mmap();
#ifdef CONFIG_MEM_RANDOM
  srand(time(0));
  uint32_t *p = (uint32_t *)pmem;
  int i;
  for (i = 0; i < (int) (MEMORY_SIZE / sizeof(p[0])); i ++) {
    p[i] = rand();
  }
#endif
}

#if CONFIG_ENABLE_MEM_DEDUP || CONFIG_USE_MMAP
void set_pmem(bool pass_pmem_from_dut, uint8_t *_pmem)
{
  if (pass_pmem_from_dut) {
    Log("Set pmem start to %p", _pmem);
    pmem = _pmem;
  } else {
    allocate_memory_with_mmap();
  }
}
#endif

/* Memory accessing interfaces */

bool check_paddr(paddr_t addr, int len, int type, int trap_type, int mode, vaddr_t vaddr) {
  if (!isa_pmp_check_permission(addr, len, type, mode)) {
    if (trap_type == MEM_TYPE_WRITE) {
      raise_access_fault(EX_SAF, vaddr);
    }else {
      Log("isa pmp check failed");
      assert(0);
      raise_read_access_fault(trap_type, vaddr);
    }
    return false;
  } else {
    return true;
  }
}

word_t paddr_read(paddr_t addr, int len, int type, int trap_type, int mode, vaddr_t vaddr) {
#ifdef CONFIG_ENABLE_IDEAL_MODEL
  cpu.im_helper.mem_access_paddr = addr;
  cpu.im_helper.mem_is_mmio = is_in_mmio(addr);
#ifdef CONFIG_DEBUG_IDEAL_MODEL
  // Logim("paddr: %lx mmio: %s", addr, cpu.im_helper.mem_is_mmio? "true" : "false");
#endif
#endif

  int cross_page_load = (mode & CROSS_PAGE_LD_FLAG) != 0;
  mode &= ~CROSS_PAGE_LD_FLAG;

  assert(type == MEM_TYPE_READ || type == MEM_TYPE_IFETCH_READ || type == MEM_TYPE_IFETCH || type == MEM_TYPE_WRITE_READ);
  if (!check_paddr(addr, len, type, trap_type, mode, vaddr)) {
    return 0;
  }
#ifndef CONFIG_SHARE
  if (likely(in_pmem(addr))) return pmem_read(addr, len);
  else {
    // check if the address is misaligned
    isa_mmio_misalign_data_addr_check(addr, vaddr, len, MEM_TYPE_READ, cross_page_load);
    if (likely(is_in_mmio(addr))) return mmio_read(addr, len);
    else raise_read_access_fault(trap_type, vaddr);
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
    // check if the address is misaligned
    isa_mmio_misalign_data_addr_check(addr, vaddr, len, MEM_TYPE_READ, cross_page_load);
#ifdef CONFIG_HAS_FLASH
#ifdef CONFIG_ENABLE_IDEAL_MODEL
    if (likely(is_in_mmio(addr))) {
      /*do nothing*/
      Logm("ideal model read paddr " FMT_PADDR " mmio do nothing\n", addr);
      return 0u;
    }
#else
    if (likely(is_in_mmio(addr))) return mmio_read(addr, len);
#endif
#endif
    if(dynamic_config.ignore_illegal_mem_access)
      return 0;
    Logm("ERROR: invalid mem read from paddr " FMT_PADDR ", NEMU raise access exception\n", addr);
    raise_read_access_fault(trap_type, vaddr);
  }
  return 0;
#endif // CONFIG_SHARE
}

#ifdef CONFIG_STORE_LOG
#ifdef CONFIG_LIGHTQS

extern uint64_t g_nr_guest_instr;

extern uint64_t stable_log_begin, spec_log_begin;

void pmem_record_store(paddr_t addr) {
  // align to 8 byte
  addr = (addr >> 3) << 3;
  uint64_t rdata = pmem_read(addr, 8);
  //assert(g_nr_guest_instr >= stable_log_begin);
  store_log_buf[store_log_ptr].inst_cnt = g_nr_guest_instr;
  store_log_buf[store_log_ptr].addr = addr;
  store_log_buf[store_log_ptr].orig_data = rdata;
  ++store_log_ptr;
  if (g_nr_guest_instr >= spec_log_begin) {
    spec_store_log_buf[spec_store_log_ptr].inst_cnt = g_nr_guest_instr;
    spec_store_log_buf[spec_store_log_ptr].addr = addr;
    spec_store_log_buf[spec_store_log_ptr].orig_data = rdata;
    ++spec_store_log_ptr;
  }
}



void pmem_record_restore(uint64_t restore_inst_cnt) {
  if (spec_log_begin <= restore_inst_cnt) {
    // use speculative rather than old stable
    memcpy(store_log_buf, spec_store_log_buf, sizeof(store_log_buf));
    store_log_ptr = spec_store_log_ptr;
  }
  for (int i = store_log_ptr - 1; i >= 0; i--) {
    if (store_log_buf[i].inst_cnt > restore_inst_cnt) {
      pmem_write(store_log_buf[i].addr, 8, store_log_buf[i].orig_data, 0);
    } else {
      break;
    }
  }
}
#else
void pmem_record_store(paddr_t addr) {
  if(dynamic_config.enable_store_log) {
    // align to 8 byte
    addr = (addr >> 3) << 3;
    uint64_t rdata = pmem_read(addr, 8);
    store_log_buf[store_log_ptr].addr = addr;
    store_log_buf[store_log_ptr].orig_data = rdata;
    ++store_log_ptr;
  }
}

void pmem_record_restore() {
  for (int i = store_log_ptr - 1; i >= 0; i--) {
    pmem_write(store_log_buf[i].addr, 8, store_log_buf[i].orig_data, 0);
  }
}
#endif // CONFIG_LIGHTQS


void pmem_record_reset() {
  store_log_ptr = 0;
}

#endif // CONFIG_STORE_LOG

void paddr_write(paddr_t addr, int len, word_t data, int mode, vaddr_t vaddr) {
#ifdef CONFIG_ENABLE_IDEAL_MODEL
  cpu.im_helper.mem_access_paddr = addr;
  cpu.im_helper.mem_is_mmio = is_in_mmio(addr);
#ifdef CONFIG_DEBUG_IDEAL_MODEL
  // Logim("paddr: %lx mmio: %s", addr, cpu.im_helper.mem_is_mmio? "true" : "false");
#endif
#endif

  int cross_page_store = (mode & CROSS_PAGE_ST_FLAG) != 0;
  // get mode's original value
  mode = mode & ~CROSS_PAGE_ST_FLAG;
  if (!check_paddr(addr, len, MEM_TYPE_WRITE, MEM_TYPE_WRITE, mode, vaddr)) {
    return;
  }
#ifndef CONFIG_SHARE
  if (likely(in_pmem(addr))) pmem_write(addr, len, data, cross_page_store);
  else {
    // check if the address is misaligned
    isa_mmio_misalign_data_addr_check(addr, vaddr, len, MEM_TYPE_WRITE, cross_page_store);
    if (likely(is_in_mmio(addr))) mmio_write(addr, len, data);
    else raise_access_fault(EX_SAF, vaddr);
  }
#else
  if (likely(in_pmem(addr))) {
#ifdef CONFIG_STORE_LOG
    pmem_record_store(addr);
#endif // CONFIG_STORE_LOG
    if(dynamic_config.debug_difftest) {
      fprintf(stderr, "[NEMU] paddr write addr:" FMT_PADDR ", data:%016lx, len:%d, mode:%d\n",
        addr, data, len, mode);
    }
    return pmem_write(addr, len, data, cross_page_store);
  } else {
    // check if the address is misaligned
    isa_mmio_misalign_data_addr_check(addr, vaddr, len, MEM_TYPE_WRITE, cross_page_store);
#ifdef CONFIG_ENABLE_IDEAL_MODEL
    if (likely(is_in_mmio(addr))) {
      /*do nothing*/
      return;
    }
#else
    if (likely(is_in_mmio(addr))) mmio_write(addr, len, data);
#endif
    else {
      if(dynamic_config.ignore_illegal_mem_access)
        return;
      printf("ERROR: invalid mem write to paddr " FMT_PADDR ", NEMU raise access exception\n", addr);
      raise_access_fault(EX_SAF, vaddr);
      return;
    }
  }
#endif
}

#ifdef CONFIG_MEMORY_REGION_ANALYSIS
bool mem_addr_use[PROGRAM_ANALYSIS_PAGES];
char *memory_region_record_file = NULL;
uint64_t get_byte_alignment(uint64_t addr) {
  addr = (addr - CONFIG_MBASE) >> ALIGNMENT_SIZE;
  return addr;
}

void analysis_memory_commit(uint64_t addr) {
  uint64_t alignment_addr = get_byte_alignment(addr);
  Assert(alignment_addr < PROGRAM_ANALYSIS_PAGES,
      "alignment set memory size is addr %lx %lx", addr, alignment_addr);
  mem_addr_use[alignment_addr] = true;
}

void analysis_use_addr_display() {
  bool display_file = false;
  if (memory_region_record_file != NULL) {
    display_file = true;
  }

  if (display_file == true) {
    FILE* fp = fopen(memory_region_record_file,"wr");
    for (int i = 0; i < PROGRAM_ANALYSIS_PAGES; i++) {
      if (mem_addr_use[i]) {
        char result[32];
        if (display_file) {
          sprintf(result, "%d\n", i);
          fputs(result,fp);
        }
      }
    }
    fclose(fp);
  } else {
    for (int i = 0; i < PROGRAM_ANALYSIS_PAGES; i++) {
      if (mem_addr_use[i]) {
        uint64_t uaddr = i << ALIGNMENT_SIZE;
        Log("use memory page%4d %lx - %lx", i, uaddr , uaddr + (1 << ALIGNMENT_SIZE) - 1);
      }
    }
  }

}

bool analysis_memory_isuse(uint64_t page) {
  assert(page < PROGRAM_ANALYSIS_PAGES);
  return mem_addr_use[page];
}
#endif

#ifdef CONFIG_DIFFTEST_STORE_COMMIT

void miss_align_store_commit_queue_push(uint64_t addr, uint64_t data, int len) {
  // align with dut
  uint8_t inside_16bytes_bound = ((addr >> 4) & 1ULL) == (((addr + len - 1) >> 4) & 1ULL);
  uint64_t st_mask = (len == 1) ? 0x1ULL : (len == 2) ? 0x3ULL : (len == 4) ? 0xfULL : (len == 8) ? 0xffULL : 0xdeadbeefULL;
  uint64_t st_data_mask = (len == 1) ? 0xffULL : (len == 2) ? 0xffffULL : (len == 4) ? 0xffffffffULL : (len == 8) ? 0xffffffffffffffffULL : 0xdeadbeefULL;
  store_commit_t low_addr_st;
  store_commit_t high_addr_st;

  if (inside_16bytes_bound) {
    low_addr_st.addr = addr - (addr % 16ULL);
    if ((addr % 16ULL) > 8) {
      low_addr_st.data = 0;
    } else {
      low_addr_st.data = (data & st_data_mask) << ((addr % 16ULL) << 3);
    }
    low_addr_st.mask = (st_mask << (addr % 16ULL)) & 0xffULL;
    low_addr_st.pc   = prev_s->pc;

    store_queue_push(low_addr_st);

    // printf("[DEBUG] inside 16 bytes region addr: %lx, data: %lx, mask: %lx\n", low_addr_st->addr, low_addr_st->data, (uint64_t)(low_addr_st->mask));
  } else {
    low_addr_st.addr = addr - (addr % 8ULL);
    low_addr_st.data = (data & (st_data_mask >> ((addr % len) << 3))) << ((8 - len + (addr % len)) << 3);
    low_addr_st.mask = (st_mask >> (addr % len)) << (8 - len + (addr % len));
    low_addr_st.pc   = prev_s->pc;

    high_addr_st.addr = addr - (addr % 16ULL) + 16ULL;
    high_addr_st.data = (data >> ((len - (addr % len)) << 3)) & (st_data_mask >> ((len - (addr % len)) << 3));
    high_addr_st.mask = st_mask >> (len - (addr % len));
    high_addr_st.pc   = prev_s->pc;

    store_queue_push(low_addr_st);
    store_queue_push(high_addr_st);

    // printf("[DEBUG] split low addr store addr: %lx, data: %lx, mask: %lx\n", low_addr_st->addr, low_addr_st->data, (uint64_t)(low_addr_st->mask));
    // printf("[DEBUG] split high addr store addr: %lx, data: %lx, mask: %lx\n", high_addr_st->addr, high_addr_st->data, (uint64_t)(high_addr_st->mask));
  }

}

void store_commit_queue_push(uint64_t addr, uint64_t data, int len, int cross_page_store) {
#ifndef CONFIG_DIFFTEST_STORE_COMMIT_AMO
  if (cpu.amo) {
    return;
  }
#endif // CONFIG_DIFFTEST_STORE_COMMIT_AMO
#ifdef CONFIG_AC_NONE
  uint8_t store_miss_align = (addr & (len - 1)) != 0;
  if (unlikely(store_miss_align)) {
    if (!cross_page_store) {
      miss_align_store_commit_queue_push(addr, data, len);
      return;
    }
  }
#endif // CONFIG_AC_NONE
  Logm("push store addr = " FMT_PADDR ", data = " FMT_WORD ", len = %d", addr, data, len);
 store_commit_t store_commit;
  uint64_t offset = addr % 8ULL;
  store_commit.addr = addr - offset;
  switch (len) {
    case 1:
      store_commit.data = (data & 0xffULL) << (offset << 3);
      store_commit.mask = 0x1 << offset;
      break;
    case 2:
      store_commit.data = (data & 0xffffULL) << (offset << 3);
      store_commit.mask = 0x3 << offset;
      break;
    case 4:
      store_commit.data = (data & 0xffffffffULL) << (offset << 3);
      store_commit.mask = 0xf << offset;
      break;
    case 8:
      store_commit.data = data;
      store_commit.mask = 0xff;
      break;
    default:
#ifdef CONFIG_AC_NONE
      // strange length, only valid from cross page write
      if (cross_page_store) {
        int i = 0;
        uint64_t _data_mask = 0;
        uint64_t _mask = 0;
        for (; i < len; i++) {
          _data_mask = (_data_mask << 8) | 0xffUL;
          _mask = (_mask << 1) | 0x1UL;
        }
        store_commit.data = (data & _data_mask) << (offset << 3);
        store_commit.mask = _mask << offset;
      } else {
        assert(0);
      }
#else
      assert(0);
#endif // CONFIG_AC_NONE
  }
  store_commit.pc = prev_s->pc;
  store_queue_push(store_commit);
}

store_commit_t store_commit_queue_pop(int *flag) {
  *flag = 1;
  store_commit_t result = {.addr=0, .data=0, .mask=0};
  if (store_queue_empty()) {
    *flag = 0;
    return result;
  }
  result = store_queue_fornt();
  store_queue_pop();
  return result;
}

int check_store_commit(uint64_t *addr, uint64_t *data, uint8_t *mask) {
  int result = 0;
  if (store_queue_empty()) {
    printf("NEMU does not commit any store instruction.\n");
    result = 1;
  }
  else {
    store_commit_t commit = store_queue_fornt();
    store_queue_pop();
    if (*addr != commit.addr || *data != commit.data || *mask != commit.mask) {
      *addr = commit.addr;
      *data = commit.data;
      *mask = commit.mask;
      result = 1;
    }
  }
  return result;
}

#endif

char *mem_dump_file = NULL;

void dump_pmem() {
  if (mem_dump_file == NULL) {
    printf("No memory dump file is specified, memory dump skipped.\n");
    return ;
  }
  FILE *fp = fopen(mem_dump_file, "wb");
  if (fp == NULL) {
    printf("Cannot open file %s, memory dump skipped.\n", mem_dump_file);
  }
  fwrite(pmem, sizeof(char), MEMORY_SIZE, fp);
}

#ifdef CONFIG_ENABLE_IDEAL_MODEL

void fm_recover_pmem_write(paddr_t addr, int len, word_t data, int cross_page_store){
  pmem_write(addr, len, data, cross_page_store);
}

#endif
