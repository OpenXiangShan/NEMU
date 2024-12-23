/***************************************************************************************
* Copyright (c) 2024, Beijing Institute of Open Source Chip
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
*
* Authors:
* - Xu, Zefan
***************************************************************************************/

// This file provides riscv timer supprot, including:
// - M-mode Timer: defined in Machine ISA
// - S-mode Timer: defined in Sstc Extension

#include <utils.h>
#include <device/alarm.h>
#include "../local-include/csr.h"

#define TIMEBASE 1000000ul
#define US_PERCYCLE (1000000 / TIMEBASE)

CSR_STRUCT_START(mtime)
CSR_STRUCT_END(mtime)

CSR_STRUCT_START(mtimecmp)
CSR_STRUCT_END(mtimecmp)

static mtime_t * mtime = NULL;
static mtimecmp_t * mtimecmp = NULL;
static uint64_t clint_mtime_correction = 0;

void init_clint();

void init_mtimer_regs(void * p_mtime, void * p_mtimecmp) {
  mtime = p_mtime;
  mtimecmp = p_mtimecmp;
}

uint64_t get_mtime() {
  return mtime->val;
}

#ifdef CONFIG_RVH
uint64_t get_htime() {
  return mtime->val + htimedelta->val;
}
#endif // CONFIG_RVH

void update_riscv_timer() {
  #ifndef CONFIG_SHARE
    #ifdef CONFIG_DETERMINISTIC
      uint64_t get_abs_instr_count();
      mtime->val = (get_abs_instr_count() / CONFIG_CYCLES_PER_MTIME_TICK) + clint_mtime_correction;
    #else // CONFIG_DETERMINISTIC
      uint64_t uptime = get_time();
      mtime->val = uptime / US_PERCYCLE + clint_mtime_correction;
    #endif // CONFIG_DETERMINISTIC
  #endif // CONFIG_SHARE
}

void set_mtime(uint64_t new_value) {
  update_riscv_timer();
  clint_mtime_correction = new_value - mtime->val;
  mtime->val = new_value;
}

void timer_wait_for_interrupt() {
#ifndef CONFIG_SHARE
  uint64_t correction = CONFIG_WFI_TIMEOUT_TICKS;

  if (get_mtime() <= mtimecmp->val) {
    correction = MIN_OF(correction, mtimecmp->val - get_mtime());
  }

  #ifdef CONFIG_RV_SSTC
  if (menvcfg->stce && get_mtime() <= stimecmp->val) {
    correction = MIN_OF(correction, stimecmp->val - get_mtime());
  }
  #endif // CONFIG_RV_SSTC

  #if defined(CONFIG_RVH) && defined(CONFIG_RV_SSTC)
  if (henvcfg->stce && get_htime() <= vstimecmp->val) {
    correction = MIN_OF(correction, vstimecmp->val - get_htime());
  }
  #endif // defined(CONFIG_RVH) && defined(CONFIG_RV_SSTC)
  
  clint_mtime_correction += correction;

  update_riscv_timer();
#endif // CONFIG_SHARE
}

word_t get_riscv_timer_interrupt() {
#ifndef CONFIG_SHARE
  mip_t tmp_mip;
  tmp_mip.val = 0;

  tmp_mip.mtip = (get_mtime() >= mtimecmp->val);

  #ifdef CONFIG_RV_SSTC
    tmp_mip.stip = (get_mtime() >= stimecmp->val) && menvcfg->stce;
  #endif // CONFIG_RV_SSTC

  #if defined(CONFIG_RVH) && defined(CONFIG_RV_SSTC)
    tmp_mip.vstip = (get_htime() >= vstimecmp->val) && henvcfg->stce;
  #endif // defined(CONFIG_RVH) && defined(CONFIG_RV_SSTC)

  return tmp_mip.val;
#endif // CONFIG_SHARE
  return 0;
}

void init_riscv_timer() {
  IFDEF(CONFIG_HAS_CLINT, init_clint());
  assert(mtime != NULL);
  assert(mtimecmp != NULL);
  add_alarm_handle(update_riscv_timer);
}
