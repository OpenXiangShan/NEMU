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

#ifndef __RISCV64_REG_H__
#define __RISCV64_REG_H__

#include <common.h>

static inline int check_reg_index(int index) {
  IFDEF(CONFIG_RT_CHECK, assert(index >= 0 && index < 32));
  return index;
}

#define reg_l(index) (cpu.gpr[check_reg_index(index)]._64)

static inline const char* reg_name(int index, int width) {
  extern const char* regsl[];
  IFDEF(CONFIG_RT_CHECK, assert(index >= 0 && index < 32));
  return regsl[index];
}

// Floating Point Regs
#define fpreg_l(index) (cpu.fpr[check_reg_index(index)]._64)

static inline const char* fpreg_name(int index, int width){
  extern const char* fpregsl[];
  IFDEF(CONFIG_RT_CHECK, assert(index >= 0 && index < 32));
  return fpregsl[index];
}

#ifdef CONFIG_RV_PMA_CSR
typedef struct {
  uint64_t base_addr;
  uint64_t range;
  uint64_t l;
  uint64_t c;
  uint64_t t; // atomic
  uint64_t a;
  uint64_t x;
  uint64_t w;
  uint64_t r;
} PMAConfig;

typedef struct PMAConfigModule {
  PMAConfig pmaconfigs[CONFIG_RV_PMA_ACTIVE_NUM];
} PMAConfigModule;
#endif
#endif
