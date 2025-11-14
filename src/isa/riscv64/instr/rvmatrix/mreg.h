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

// 128 * 64 * 8
#define MLEN 65536

// 64 * 8
#define MRLEN 512

#define MELEN 32
#define MAMUL 4     // It's sufficient to set MAMUL=4 for fp8 -> fp32
#define MLENB (MLEN/8)

#define TRMLEN  MLEN
#define TRMRLEN MRLEN

#define TRMRNUM   (TRMLEN/TRMRLEN)
#define TRMRENUM64 (TRMRLEN/64)
#define TRMRENUM32 (TRMRLEN/32)
#define TRMRENUM16 (TRMRLEN/16)
#define TRMRENUM8  (TRMRLEN/8)

#define ACCMLEN ((MLEN/MRLEN)*(MLEN/MRLEN)*MELEN)
#define ACCMRLEN ((MLEN/MRLEN)*MELEN)

#define ACCMRNUM (ACCMLEN/ACCMRLEN)
#define ACCMRENUM64 (ACCMRLEN/64)
#define ACCMRENUM32 (ACCMRLEN/32)
#define ACCMRENUM16 (ACCMRLEN/16)
#define ACCMRENUM8  (ACCMRLEN/8)

#define TMMAX      (MLEN/MRLEN)
#define TNMAX      (MLEN/MRLEN)
#define TKMAX(sew) (MRLEN/sew)   // 512/8=64, init_SEW=MELEN

#define MTOK       32

static inline int check_mreg_num(int num) {
  assert(num >= 0 && num < 8);
  return num;
}

static inline int check_mtreg_row(int row) {
  assert(row >= 0 && row < TRMRNUM);
  return row;
}

static inline int check_macc_row(int row) {
  assert(row >= 0 && row < ACCMRNUM);
  return row;
}

static inline int check_mtreg_idx(int idx, int elen) {
  assert(idx >= 0 && idx < TRMRLEN/elen);
  return idx;
}

static inline int check_macc_idx(int idx, int elen) {
  assert(idx >= 0 && idx < ACCMRLEN/elen);
  return idx;
}

#define mtreg_l64(num, row, idx) (cpu.mtr[check_mreg_num(num)][check_mtreg_row(row)]._64[check_mtreg_idx(idx, 64)])
#define mtreg_l32(num, row, idx) (cpu.mtr[check_mreg_num(num)][check_mtreg_row(row)]._32[check_mtreg_idx(idx, 32)])
#define mtreg_l16(num, row, idx) (cpu.mtr[check_mreg_num(num)][check_mtreg_row(row)]._16[check_mtreg_idx(idx, 16)])
#define mtreg_l8(num, row, idx)  (cpu.mtr[check_mreg_num(num)][check_mtreg_row(row)]._8[check_mtreg_idx(idx, 8)])
#define macc_l64(num, row, idx)  (cpu.macc[check_mreg_num(num)][check_macc_row(row)]._64[check_macc_idx(idx, 64)])
#define macc_l32(num, row, idx)  (cpu.macc[check_mreg_num(num)][check_macc_row(row)]._32[check_macc_idx(idx, 32)])
#define macc_l16(num, row, idx)  (cpu.macc[check_mreg_num(num)][check_macc_row(row)]._16[check_macc_idx(idx, 16)])
#define macc_l8(num, row, idx)   (cpu.macc[check_mreg_num(num)][check_macc_row(row)]._8[check_macc_idx(idx, 8)])

// isacc == false for tile register
// isacc == true for acc register
void set_mreg(bool isacc, int mtr_num, int mtr_row, int mtr_idx, rtlreg_t src_data, uint64_t msew);
void get_mreg(bool isacc, int mtr_num, int mtr_row, int mtr_idx, rtlreg_t *dst, uint64_t msew, bool is_signed);

#endif //__RISCV64_MREG_H__

#endif // CONFIG_RVMATRIX
