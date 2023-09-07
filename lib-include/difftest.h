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

#ifndef __DIFFTEST_H__
#define __DIFFTEST_H__

#include <stdint.h>

enum { DIFFTEST_TO_DUT, DIFFTEST_TO_REF };

#define RV64_FULL_DIFF
#define RV64_UARCH_SYNC

#if defined(__ISA_x86__)
# define DIFFTEST_REG_SIZE (sizeof(uint32_t) * 9) // GPRs + PC
#elif defined(__ISA_mips32__)
# define DIFFTEST_REG_SIZE (sizeof(uint32_t) * 38) // GRPs + status + lo + hi + badvaddr + cause + pc
#elif defined(__ISA_riscv32__)
# define DIFFTEST_REG_SIZE (sizeof(uint32_t) * 33) // GRPs + pc
#elif defined(__ISA_riscv64__)
#ifdef RV64_FULL_DIFF
#ifdef CONFIG_FPU_NONE
#define BASE_SIZE (sizeof(uint64_t) * (32 + 1 + 6 + 11 + 1))
#else
#define BASE_SIZE (sizeof(uint64_t) * (32 + 32 + 1 + 6 + 11 + 1))
#endif
// GRPs + FPRs + pc + [m|s][status|cause|epc] + other necessary CSRs + mode
#else
#define BASE_SIZE (sizeof(uint64_t) * (32 + 1)) // GRPs + pc
#endif //RV64_FULL_DIFF

#if defined (RV64_FULL_DIFF) && defined (CONFIG_RVV)
#define RVV_EXT_REG_SIZE (sizeof(uint64_t) * (64 + 7))
#else
#define RVV_EXT_REG_SIZE 0
#endif //CONFIG_RVV

#if defined (RV64_FULL_DIFF) && defined (CONFIG_RVH)
#define RVH_EXT_REG_SIZE (sizeof(uint64_t) * (1 + 16)) // v-mode + HCSRS
#else
#define RVH_EXT_REG_SIZE 0
#endif //CONFIG_RVH

#define DIFFTEST_REG_SIZE (BASE_SIZE + RVH_EXT_REG_SIZE + RVV_EXT_REG_SIZE)

#else
# error Unsupported ISA
#endif

#ifdef RV64_UARCH_SYNC
struct SyncState {
  uint64_t lrscValid;
  uint64_t lrscAddr;
};
#endif

#endif
