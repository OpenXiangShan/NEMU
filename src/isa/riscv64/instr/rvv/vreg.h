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

#include <common.h>
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
#ifdef CONFIG_RVV_AGNOSTIC
#define RVV_AGNOSTIC 1
#else
#define RVV_AGNOSTIC 0
#endif

#define RNU 0
#define RNE 1
#define RDN 2
#define ROD 3

typedef union {
  uint64_t _64[VENUM64];
  uint32_t _32[VENUM32];
  uint16_t _16[VENUM16];
  uint8_t  _8[VENUM8];
} rtlvreg_t;

extern rtlvreg_t tmp_vreg[8];

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

rtlreg_t get_mask(int reg, int idx, uint64_t vsew, uint64_t vlmul);

static inline const char * vreg_name(int index, int width) {
  extern const char * vregsl[];
  assert(index >=  0 && index < 32);
  return vregsl[index];
}

int get_vlmax(int vsew, int vlmul);
int get_vlen_max(int vsew, int vlmul);
void get_vreg(uint64_t reg, int idx, rtlreg_t *dst, uint64_t vsew, uint64_t vlmul, int is_signed, int needAlign);
void set_vreg(uint64_t reg, int idx, rtlreg_t src, uint64_t vsew, uint64_t vlmul, int needAlgin);

void init_tmp_vreg();
void get_tmp_vreg(uint64_t reg, int idx, rtlreg_t *dst, uint64_t vsew);
void set_tmp_vreg(uint64_t reg, int idx, rtlreg_t src, uint64_t vsew);
void vreg_to_tmp_vreg(uint64_t reg, int idx, uint64_t vsew);

void set_vreg_tail(uint64_t reg);

void longjmp_raise_intr(uint32_t foo);

#define SRC_VV  0
#define SRC_VI  1
#define SRC_VX  2
#define SRC_VF  3
#define SRC_V   4
#define UNSIGNED     0
#define SIGNED       1

// set vp dirty
#define set_mstatus_dirt() \
do{ \
  if(((mstatus->val >> 9) & 3ull) != 3) {\
    mstatus->val = mstatus->val | (3ull << 9);\
    mstatus->sd  = 1;\
  } \
} while (0) \

void vcsr_write(uint32_t addr,  rtlreg_t *src);
void vcsr_read(uint32_t addr, rtlreg_t *dest);

#endif //__RISCV64_VREG_H__

#endif // CONFIG_RVV