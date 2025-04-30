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
#include <memory/host.h>
#include <memory/store_queue_wrapper.h>
#include <memory/sparseram.h>
#include <cpu/cpu.h>
#include <difftest.h>

extern void load_flash_contents(const char *flash_img);

#ifdef CONFIG_LARGE_COPY
#ifndef CONFIG_USE_SPARSEMM
static void* nemu_large_memcpy(void *dest, const void *src, size_t n) {
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
  return dest;
}
#endif
#endif

#ifdef CONFIG_USE_SPARSEMM
void nemu_sparse_mem_copy(paddr_t nemu_addr, void *dut_buf, size_t n, bool direction) {
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
}
#endif

void nemu_memcpy_helper(paddr_t nemu_addr, void *dut_buf, size_t n, bool direction, void* (*cpy_func)(void*, const void*, size_t)) {
  assert(guest_to_host(nemu_addr) != NULL);
  if (direction == DIFFTEST_TO_REF) cpy_func(guest_to_host(nemu_addr), dut_buf, n);
  else cpy_func(dut_buf, guest_to_host(nemu_addr), n);
}

void difftest_get_backed_memory(void *backed_pmem, size_t n) {
#if CONFIG_ENABLE_MEM_DEDUP
  // set pmem to backed_pmem, then nothing
  assert(n == CONFIG_MSIZE);
  set_pmem(true, backed_pmem);
#endif
}

void difftest_memcpy_init(paddr_t nemu_addr, void *dut_buf, size_t n, bool direction) {
#ifdef CONFIG_USE_SPARSEMM
  nemu_sparse_mem_copy(nemu_addr, dut_buf, n, direction);
#else
#ifdef CONFIG_LARGE_COPY
  nemu_memcpy_helper(nemu_addr, dut_buf, n, direction, nemu_large_memcpy);
#else
  nemu_memcpy_helper(nemu_addr, dut_buf, n, direction, memcpy);
#endif
#endif
}

void difftest_memcpy(paddr_t nemu_addr, void *dut_buf, size_t n, bool direction) {
#ifdef CONFIG_USE_SPARSEMM
  nemu_sparse_mem_copy(nemu_addr, dut_buf, n, direction);
#else
  nemu_memcpy_helper(nemu_addr, dut_buf, n, direction, memcpy);
#endif
}

void difftest_load_flash(void *flash_bin, size_t f_size){
#ifndef CONFIG_HAS_FLASH
  printf("nemu does not enable flash fetch!\n");
#else
  load_flash_contents((const char *)flash_bin);
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
#ifdef CONFIG_LIGHTQS
  // This just fixes compilation error for lightqs.
  // No guarantee given for functional correctness.
  isa_difftest_uarchstatus_cpy(dut, DIFFTEST_TO_REF, 0);
#else
  isa_difftest_uarchstatus_cpy(dut, DIFFTEST_TO_REF);
#endif // CONFIG_LIGHTQS
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
#ifdef CONFIG_RV_SMDBLTRP
bool difftest_raise_critical_error() {
  return cpu.critical_error;
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
  return br_log_query();
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

void difftest_raise_nmi_intr(bool hasNMI) {
#ifdef CONFIG_RV_SMRNMI
  cpu.hasNMI = hasNMI;
#endif //CONFIG_RV_SMRNMI
}

void difftest_virtual_interrupt_is_hvictl_inject(bool virtualInterruptIsHvictlInject) {
#ifdef CONFIG_RV_IMSIC
  cpu.virtualInterruptIsHvictlInject = virtualInterruptIsHvictlInject;
#endif
}

void difftest_raise_mhpmevent_overflow(uint64_t mhpmeventOverflowVec) {
#ifdef CONFIG_RV_SSCOFPMF
  isa_update_mhpmcounter_overflow(mhpmeventOverflowVec);
#endif
}

void difftest_non_reg_interrupt_pending(void *nonRegInterruptPending) {
  memcpy(&cpu.non_reg_interrupt_pending, nonRegInterruptPending, sizeof(struct NonRegInterruptPending));
  isa_update_mip(cpu.non_reg_interrupt_pending.lcofi_req);
#ifdef CONFIG_RV_IMSIC
  if (cpu.non_reg_interrupt_pending.platform_irp_meip || cpu.non_reg_interrupt_pending.from_aia_meip ||
      cpu.non_reg_interrupt_pending.platform_irp_seip || cpu.non_reg_interrupt_pending.from_aia_seip) {
    isa_update_external_interrupt_select();
  }
  isa_update_mtopi();
  isa_update_stopi();
  isa_update_vstopi();
#endif
}

void difftest_interrupt_delegate(void *interruptDelegate) {
#ifdef CONFIG_RV_IMSIC
  memcpy(&cpu.interrupt_delegate, interruptDelegate, sizeof(struct InterruptDelegate));
#endif // CONFIG_RV_IMSIC
}

#ifdef CONFIG_DIFFTEST_STORE_COMMIT
void difftest_get_store_event_other_info(void *info) {
  *(uint64_t*)info = get_store_commit_info().pc;
}
#endif //CONFIG_DIFFTEST_STORE_COMMIT



#if defined(CONFIG_MULTICORE_DIFF) && defined(CONFIG_RVV)
extern uint32_t vec_laod_mul;
int difftest_get_vec_load_vdNum() {
  return vec_laod_mul;
}

void *difftest_get_vec_load_dual_goldenmem_reg(void *regPtr) {
  return get_vec_dual_reg();
}

extern uint64_t vec_load_difftest_addr_queue[128];
extern uint64_t vec_load_difftest_data_queue[128];
extern uint8_t  vec_load_difftest_len_queue[128];
extern uint32_t vec_load_difftest_info_queue_cnt;
void difftest_update_vec_load_pmem() {
  for (uint32_t i = 0; i < vec_load_difftest_info_queue_cnt; i++) {
    host_write(guest_to_host(vec_load_difftest_addr_queue[i]), vec_load_difftest_len_queue[i], vec_load_difftest_data_queue[i]);
  }
}
#endif // defined(CONFIG_MULTICORE_DIFF) && defined(CONFIG_RVV)


void difftest_sync_aia(void *src) {
#ifdef CONFIG_RV_IMSIC
  memcpy(&cpu.fromaia, src, sizeof(struct FromAIA));
  isa_update_mtopi();
  isa_update_stopi();
  isa_update_vstopi();
  isa_update_hgeip();
#endif
}

void difftest_sync_custom_mflushpwr(bool l2FlushDone) {
  isa_sync_custom_mflushpwr(l2FlushDone);
}

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
#ifdef CONFIG_SHARE_OUTPUT_LOG_TO_FILE
  char log_file_name[20];
  sprintf(log_file_name, "nemu-hart-%d.log", PMEM_HARTID);
  void init_log(const char *log_file, const bool fast_log, const bool small_log);
  init_log(log_file_name, false, false);
#endif // CONFIG_SHARE_OUTPUT_LOG_TO_FILE

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
  PMEM_HARTID = n;
}

void difftest_put_gmaddr(uint8_t* ptr) {
  golden_pmem = ptr;
}

#endif

#ifdef CONFIG_STORE_LOG
// This just fixes compilation error for lightqs.
// No guarantee given for functional correctness.
#ifndef CONFIG_LIGHTQS
void difftest_store_log_reset() {
  extern void pmem_record_reset();
  pmem_record_reset();
}

void difftest_store_log_restore() {
  extern void pmem_record_restore();
  pmem_record_restore();
}
#endif
#endif // CONFIG_STORE_LOG
