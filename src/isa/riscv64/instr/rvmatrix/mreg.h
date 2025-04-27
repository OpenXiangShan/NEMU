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

#define MLEN 524288 //tmp: to support 64*256*64, we have to use a 256*256*sew(8) tr register
#define MRLEN 2048
#define MELEN 64
#define MAMUL 4     // It's sufficient to set MAMUL=4 for fp8 -> fp32
#define MLENB (MLEN/8)

#define MRNUM   (MLEN/MRLEN)
#define MRENUM64 (MRLEN/64)
#define MRENUM32 (MRLEN/32)
#define MRENUM16 (MRLEN/16)
#define MRENUM8  (MRLEN/8)

#define TMMAX      64    // 8
#define TNMAX(sew) 512/sew  // 512/8=64, add sew to make compiler happy
#define TKMAX(sew) 2048/sew   // 2048/8=256, init_SEW=MELEN


static inline int check_mtreg_num(int num) {
  assert(num >= 0 && num < 8);
  return num;
}

static inline int check_macc_num(int num) {
  return check_mtreg_num(num); // the same as mtreg
}

static inline int check_mtreg_row(int row) {
  assert(row >= 0 && row < MRNUM);
  return row;
}

static inline int check_macc_row(int row) {
  return check_mtreg_row(row); // the same as mtreg
}

static inline int check_mtreg_idx(int idx, int elen) {
  assert(idx >= 0 && idx < MRLEN/elen);
  return idx;
}

static inline int check_macc_idx(int idx, int elen) {
  assert(idx >= 0 && idx < MRLEN * MAMUL / elen);
  return idx;
}

#define mtreg_l64(num, row, idx) (cpu.mtr[check_mtreg_num(num)][check_mtreg_row(row)]._64[check_mtreg_idx(idx, 64)])
#define mtreg_l32(num, row, idx) (cpu.mtr[check_mtreg_num(num)][check_mtreg_row(row)]._32[check_mtreg_idx(idx, 32)])
#define mtreg_l16(num, row, idx) (cpu.mtr[check_mtreg_num(num)][check_mtreg_row(row)]._16[check_mtreg_idx(idx, 16)])
#define mtreg_l8(num, row, idx) (cpu.mtr[check_mtreg_num(num)][check_mtreg_row(row)]._8[check_mtreg_idx(idx, 8)])
#define macc_l64(num, row, idx) (cpu.macc[check_macc_num(num)][check_macc_row(row)]._64[check_macc_idx(idx, 64)])
#define macc_l32(num, row, idx) (cpu.macc[check_macc_num(num)][check_macc_row(row)]._32[check_macc_idx(idx, 32)])
#define macc_l16(num, row, idx) (cpu.macc[check_macc_num(num)][check_macc_row(row)]._16[check_macc_idx(idx, 16)])
#define macc_l8(num, row, idx) (cpu.macc[check_macc_num(num)][check_macc_row(row)]._8[check_macc_idx(idx, 8)])

// isacc == false for tile register
// isacc == true for acc register
void set_mreg(bool isacc, int mtr_num, int mtr_row, int mtr_idx, rtlreg_t src_data, uint64_t msew);
void get_mreg(bool isacc, int mtr_num, int mtr_row, int mtr_idx, rtlreg_t *dst, uint64_t msew, bool is_signed);

#endif //__RISCV64_MREG_H__

#endif // CONFIG_RVMATRIX
