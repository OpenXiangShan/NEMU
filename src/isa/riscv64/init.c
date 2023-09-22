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
#include "local-include/csr.h"

#ifndef CONFIG_SHARE
static const uint32_t img [] = {
  0x800002b7,  // lui t0,0x80000
  0x0002a023,  // sw  zero,0(t0)
  0x0002a503,  // lw  a0,0(t0)
  0x0000006b,  // nemu_trap
};
#endif

void init_csr();
#ifdef CONFIG_RVSDTRIG
void init_trigger();
#endif

#if !defined(CONFIG_SHARE) || defined(CONFIG_LIGHTQS)
void init_clint();
#endif
void init_device();

void init_isa() {
  // NEMU has some cached states and some static variables in the source code.
  // They are assumed to have initialized states every time when the dynamic lib is loaded.
  // However, if we link NEMU as a static library, we have to manually initialize them.
  static bool is_second_call = false;
  if (is_second_call) {
    memset(csr_array, 0, sizeof(csr_array));
  }
  init_csr();

#ifndef CONFIG_RESET_FROM_MMIO
  cpu.pc = RESET_VECTOR;
#else
  cpu.pc = CONFIG_MMIO_RESET_VECTOR;
#endif
  cpu.lr_valid = 0;

  cpu.gpr[0]._64 = 0;

  cpu.mode = MODE_M;
  // For RV64 systems, the SXL and UXL fields are WARL fields that
  // control the value of XLEN for S-mode and U-mode, respectively.
  // For RV64 systems, if S-mode is not supported, then SXL is hardwired to zero.
  // For RV64 systems, if U-mode is not supported, then UXL is hardwired to zero.
  mstatus->val = 0xaUL << 32;

#ifdef CONFIG_RV_PMP_CSR
  pmpcfg0->val = 0;
  pmpcfg2->val = 0;
#endif // CONFIG_RV_PMP_CSR

#ifdef CONFIG_RV_SVINVAL
  srnctl->val = 3; // enable extension 'svinval' [1]
#endif

#define ext(e) (1 << ((e) - 'a'))
  misa->extensions = ext('i') | ext('m') | ext('a') | ext('c') | ext('s') | ext('u');
#ifndef CONFIG_FPU_NONE
  misa->extensions |= ext('d') | ext('f');
#endif // CONFIG_FPU_NONE
#ifdef CONFIG_RVH
  misa->extensions |= ext('h');
  hstatus->vsxl = 2; // equal to max len (spike)
  vsstatus->val = mstatus->val & SSTATUS_RMASK;
  mideleg->val |= ((1 << 12) | (1 << 10) | (1 << 6) | (1 << 2));
#endif

  misa->mxl = 2; // XLEN = 64

#ifdef CONFIG_RVV
  // vector
  misa->extensions |= ext('v');
  vl->val = 0;
  vtype->val = (uint64_t) 1 << 63; // actually should be 1 << 63 (set vill bit to forbidd)
  vlenb->val = VLEN/8;
#endif // CONFIG_RVV

#ifdef CONFIG_RV_ARCH_CSRS
  #ifdef CONFIG_USE_XS_ARCH_CSRS
    mvendorid->val = 0;
    marchid->val = 25;
    mimpid->val = 0;
  #else
    mvendorid->val = CONFIG_MVENDORID_VALUE;
    marchid->val = MUXDEF(CONFIG_DIFFTEST_REF_SPIKE, 0x5, CONFIG_MARCHID_VALUE);
    mimpid->val = CONFIG_MIMPID_VALUE;
  #endif // CONFIG_USE_XS_ARCH_CSRS
#endif // CONFIG_RV_ARCH_CSRS

#ifdef CONFIG_RVSDTRIG
  init_trigger();
#endif // CONFIG_RVSDTRIG

#ifndef CONFIG_SHARE
  extern char *cpt_file;
  if (cpt_file == NULL) {
    memcpy(guest_to_host(RESET_VECTOR), img, sizeof(img));
  }
#endif

  #if defined(CONFIG_LIGHTQS) || !defined(CONFIG_SHARE)
  init_clint();
  #endif

  if (!is_second_call) {
    IFDEF(CONFIG_SHARE, init_device());
  }

#ifndef CONFIG_SHARE
  Log("NEMU will start from pc 0x%lx", cpu.pc);
#endif

  csr_prepare();

  is_second_call = true;
}
