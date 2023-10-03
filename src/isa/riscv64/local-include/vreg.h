/***************************************************************************************
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

#ifdef CONFIG_RVV
#ifndef __RISCV64_VREG_H__
#define __RISCV64_VREG_H__

#include "common.h"

#define VLEN 128
#define VLENLG 8
#define MAXELEN 64
#define VENUM64 (VLEN/64)
#define VENUM32 (VLEN/32)
#define VENUM16 (VLEN/16)
#define VENUM8  (VLEN/8)
#define SLEN 256

static inline int check_reg_index1(int index) {
  assert(index >= 0 && index < 32);
  return index;
}

static inline int check_reg_index2(int index2, int elen) {
  assert(index2 >= 0 && index2 < VLEN/elen);
  return index2;
}

// #define vreg_len(index1, index2, elen) (cpu.vr[check_reg_index1(index1)]._64[check_reg_index2(index2, elen)])
#define vreg_ll(index) (cpu.vr[check_reg_index1(index)])
// #define vreg_l(index1, index2) vreg_len(index1, index2, 64)
// #define vreg_i(index1, index2) vreg_len(index1, index2, 32)
// #define vreg_s(index1, index2) vreg_len(index1, index2, 16)
// #define vreg_b(index1, index2) vreg_len(index1, index2,  8)
#define vreg_l(index1, index2) (cpu.vr[check_reg_index1(index1)]._64[check_reg_index2(index2, 64)])
#define vreg_i(index1, index2) (cpu.vr[check_reg_index1(index1)]._32[check_reg_index2(index2, 32)])
#define vreg_s(index1, index2) (cpu.vr[check_reg_index1(index1)]._16[check_reg_index2(index2, 16)])
#define vreg_b(index1, index2) (cpu.vr[check_reg_index1(index1)]._8[check_reg_index2(index2,   8)])


static inline const char * vreg_name(int index, int width) {
  extern const char * vregsl[];
  assert(index >=  0 && index < 32);
  return vregsl[index];
}

void get_vreg(uint64_t reg, int idx, rtlreg_t *dst, uint64_t vsew, uint64_t vlmul, int is_signed, int needAlign);
void set_vreg(uint64_t reg, int idx, rtlreg_t src, uint64_t vsew, uint64_t vlmul, int needAlgin);

void set_vreg_tail(uint64_t reg);

void longjmp_raise_intr(uint32_t foo);

#define SRC_VV  0
#define SRC_VI  1
#define SRC_VX  2
#define UNSIGNED     0
#define SIGNED       1
#endif //__RISCV64_VREG_H__

#endif // CONFIG_RVV