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

#ifdef CONFIG_RVMATRIX

#ifndef __RISCV64_MREG_H__
#define __RISCV64_MREG_H__

#include "common.h"

#define MLEN 1024
#define MRLEN 128
#define MELEN 64
#define MAMUL 8
#define MLENB (MLEN/8)

#define MRNUM   (MLEN/MRLEN)
#define MRENUM64 (MRLEN/64)
#define MRENUM32 (MRLEN/32)
#define MRENUM16 (MRLEN/16)
#define MRENUM8  (MRLEN/8)

#define TMMAX      (MLEN/MRLEN)    // 8
#define TNMAX(sew) (MRLEN/sew)  // 16/8/4/2
#define TKMAX(sew) (TMMAX < TNMAX(sew) ? TMMAX : TNMAX(sew))   // min{MLEN/RLEN, RLEN/SEW}, SEW=8/16/32/64, init_SEW=MELEN


static inline int check_mtreg_num(int num) {
  assert(num >= 0 && num < 8);
  return num;
}

static inline int check_mtreg_row(int row) {
  assert(row >= 0 && row < MRNUM);
  return row;
}

static inline int check_mtreg_idx(int idx, int elen) {
  assert(idx >= 0 && idx < MRLEN/elen);
  return idx;
}

#define mtreg_l64(num, row, idx) (cpu.mtr[check_mtreg_num(num)][check_mtreg_row(row)]._64[check_mtreg_idx(idx, 64)])
#define mtreg_l32(num, row, idx) (cpu.mtr[check_mtreg_num(num)][check_mtreg_row(row)]._32[check_mtreg_idx(idx, 32)])
#define mtreg_l16(num, row, idx) (cpu.mtr[check_mtreg_num(num)][check_mtreg_row(row)]._16[check_mtreg_idx(idx, 16)])
#define mtreg_l8(num, row, idx) (cpu.mtr[check_mtreg_num(num)][check_mtreg_row(row)]._8[check_mtreg_idx(idx, 8)])

void set_mtreg(int mtr_num, int mtr_row, int mtr_idx, rtlreg_t src_data, uint64_t msew);
void get_mtreg(int mtr_num, int mtr_row, int mtr_idx, rtlreg_t *dst, uint64_t msew, bool is_signed);

#endif //__RISCV64_MREG_H__

#endif // CONFIG_RVMATRIX
