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
#include <memory/paddr.h>
#include <memory/sparseram.h>
#include <cpu/cpu.h>
#include <difftest.h>

extern void init_flash();
extern void load_flash_contents(const char *flash_img);


#ifdef CONFIG_LARGE_COPY
#ifndef CONFIG_USE_SPARSEMM
static void nemu_large_memcpy(void *dest, void *src, size_t n) {
  uint64_t *_dest = (uint64_t *)dest;
  uint64_t *_src  = (uint64_t *)src;
  while (n >= sizeof(uint64_t)) {
    if (*_src != 0) {
      *_dest = *_src;
    }
    _dest++;
    _src++;
    n -= sizeof(uint64_t);
  }
  if (n > 0) {
    uint8_t *dest8 = (uint8_t *)_dest;
    uint8_t *src8  = (uint8_t *)_src;
    while (n > 0) {
      *dest8 = *src8;
      dest8++;
      src8++;
      n--;
    }
  }
}
#endif
#endif

void difftest_memcpy(paddr_t nemu_addr, void *dut_buf, size_t n, bool direction) {
#ifdef CONFIG_USE_SPARSEMM
  void *a = get_sparsemm();
  printf("[sp-mem] copy sparse mm: %p -> %p with direction %s\n",
          dut_buf, a, direction == DIFFTEST_TO_REF ? "DIFFTEST_TO_REF": "REF_TO_DIFFTEST");
  if (direction == DIFFTEST_TO_REF)sparse_mem_copy(a, dut_buf);
  else sparse_mem_copy(dut_buf, a);

  printf("[sp-mem] copy complete, eg data: ");
  for (int j = 0; j < 8; j++) {
    printf("%016lx", sparse_mem_wread(a, nemu_addr + j*sizeof(uint64_t), sizeof(uint64_t)));
  }
  printf("\n");

#else
#ifdef CONFIG_LARGE_COPY
  if (direction == DIFFTEST_TO_REF) nemu_large_memcpy(guest_to_host(nemu_addr), dut_buf, n);
  else nemu_large_memcpy(dut_buf, guest_to_host(nemu_addr), n);
#else
  if (direction == DIFFTEST_TO_REF) memcpy(guest_to_host(nemu_addr), dut_buf, n);
  else memcpy(dut_buf, guest_to_host(nemu_addr), n);
#endif
#endif
}

void difftest_load_flash(void *flash_bin, size_t f_size){
#ifndef CONFIG_HAS_FLASH
  printf("nemu does not enable flash fetch!\n");
#else
  load_flash_contents((const char *)flash_bin);
  init_flash();
#endif
}

void difftest_set_ramsize(size_t ram_size){
  // should be called before difftest_init()
#ifdef CONFIG_USE_MMAP
  if(ram_size){
    MEMORY_SIZE = ram_size;
  }else{
  }
#else
  printf("Set CONFIG_USE_MMAP to enable configurable memory size\n");
  printf("NEMU memory size remain unchanged\n");
#endif
}
#ifdef CONFIG_LIGHTQS
void difftest_regcpy(void *dut, bool direction, bool restore, uint64_t restore_count) {
  if (restore)
    #ifdef CONFIG_LIGHTQS_DEBUG
    printf("regcpy with restore called, dut = %lx restore = %d, restore_count = %lu\n", (uint64_t)dut, restore, restore_count);
    #endif // CONFIG_LIGHTQS_DEBUG
  isa_difftest_regcpy(dut, direction, restore, restore_count);
}
#else
void difftest_regcpy(void *dut, bool direction) {
  isa_difftest_regcpy(dut, direction);
}
#endif // CONFIG_LIGHTQS

#ifdef RV64_FULL_DIFF
void difftest_csrcpy(void *dut, bool direction) {
  isa_difftest_csrcpy(dut, direction);
}

void difftest_uarchstatus_sync(void *dut) {
  isa_difftest_uarchstatus_cpy(dut, DIFFTEST_TO_REF);
}

#ifdef CONFIG_LIGHTQS
void difftest_uarchstatus_cpy(void *dut, bool direction, uint64_t restore_count) {
  isa_difftest_uarchstatus_cpy(dut, direction, restore_count);
}
#else
void difftest_uarchstatus_cpy(void *dut, bool direction) {
  isa_difftest_uarchstatus_cpy(dut, direction);
}
#endif // CONFIG_LIGHTQS

int difftest_store_commit(uint64_t *saddr, uint64_t *sdata, uint8_t *smask) {
#ifdef CONFIG_DIFFTEST_STORE_COMMIT
  return check_store_commit(saddr, sdata, smask);
#else
  return 0;
#endif
}
#endif

void difftest_exec(uint64_t n) {
  cpu_exec(n);
}

#ifdef CONFIG_REF_STATUS
int difftest_status() {
  switch (nemu_state.state) {
    case NEMU_RUNNING: case NEMU_QUIT:
      return 0;
    case NEMU_END:
      return (nemu_state.halt_ret == 0) ? 6 : 1;
    default:
      return 1;
  }
}
#endif

#ifdef CONFIG_LIGHTQS
void difftest_guided_exec(void * guide, uint64_t restore_count) {
#ifdef CONFIG_LIGHTQS_DEBUG
  printf("guided exec called\n");
#endif // LIGHTQS_DEBUG
#ifdef CONFIG_GUIDED_EXEC
  isa_difftest_guided_exec(guide, restore_count);
#else // CONFIG_GUIDED_EXEC
  difftest_exec(1);
#endif // CONFIG_GUIDED_EXEC
}
#else // CONFIG_LIGHTQS
void difftest_guided_exec(void * guide) {
#ifdef CONFIG_GUIDED_EXEC
  isa_difftest_guided_exec(guide);
#else // CONFIG_GUIDED_EXEC
  difftest_exec(1);
#endif // CONFIG_GUIDED_EXEC
}
#endif // CONFIG_LIGHTQS

#ifdef CONFIG_BR_LOG
void *difftest_query_br_log() {
  return (void *)isa_difftest_query_br_log();
}
#endif // CONFIG_BR_LOG

#ifdef CONFIG_QUERY_REF
void difftest_query_ref(void * result_buffer, uint64_t type) {
  isa_difftest_query_ref(result_buffer, type);
}
#endif

#ifdef CONFIG_LIGHTQS
void difftest_raise_intr(word_t NO, uint64_t restore_count) {
  #ifdef CONFIG_LIGHTQS_DEBUG
  printf("raise intr called\n");
  #endif // CONFIG_LIGHTQS_DEBUG
  isa_difftest_raise_intr(NO, restore_count);
}
#else // CONFIG_LIGHTQS
void difftest_raise_intr(word_t NO) {
  isa_difftest_raise_intr(NO);
}
#endif // CONFIG_LIGHTQS

void difftest_enable_debug() {
#ifdef CONFIG_SHARE
  dynamic_config.debug_difftest = true;
#endif
}

void difftest_runahead_init() {
#ifdef CONFIG_SHARE
#ifdef CONFIG_LIGHTQS
  extern uint64_t stable_log_begin, spec_log_begin;
  stable_log_begin = 0;
  spec_log_begin = AHEAD_LENGTH;
  lightqs_take_reg_snapshot();
  // clint_take_snapshot();
  cpu_exec(AHEAD_LENGTH);
  lightqs_take_spec_reg_snapshot();
  // clint_take_spec_snapshot();
#endif // CONFIG_LIGHTQS
#endif // CONFIG_SHARE
}

void difftest_init() {
  init_mem();

  /* Perform ISA dependent initialization. */
  init_isa();
  /* create dummy address space for serial */
  //add_mmio_map("difftest.serial", 0xa10003f8, new_space(8), 8, NULL);

#ifdef CONFIG_SHARE
  dynamic_config.debug_difftest = false;
#endif
}

void difftest_display() {
  isa_reg_display();
}

#ifdef CONFIG_MULTICORE_DIFF
uint8_t *golden_pmem = NULL;

void difftest_set_mhartid(int n) {
  isa_difftest_set_mhartid(n);
}

void difftest_put_gmaddr(uint8_t* ptr) {
  golden_pmem = ptr;
}

#endif
