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

#ifdef CONFIG_RV_AME

#ifndef __RISCV64_MREG_H__
#define __RISCV64_MREG_H__

#include "common.h"

#define TLEN   CONFIG_RV_AME_TLEN
#define TRLEN  CONFIG_RV_AME_TRLEN
#define MELEN  CONFIG_RV_AME_MLEN

#define ROWNUM (TLEN / TRLEN)

#define TRENUM64 (TRLEN / 64)
#define TRENUM32 (TRLEN / 32)
#define TRENUM16 (TRLEN / 16)
#define TRENUM8  (TRLEN / 8)

#define ALEN     (ROWNUM * ROWNUM * MELEN)
#define ARLEN    (ROWNUM * MELEN)

#define ARENUM64 (ARLEN/64)
#define ARENUM32 (ARLEN/32)
#define ARENUM16 (ARLEN/16)
#define ARENUM8  (ARLEN/8)

#define TMMAX      ROWNUM
#define TNMAX      ROWNUM
#define TKMAX(sew) (TRLEN/sew)

#define MSYNC       CONFIG_RV_AME_MSYNC

enum MTYPECODE_TAB0 {
  MTYPECODE_INT4 = 0,
  MTYPECODE_UINT4,
  MTYPECODE_INT8,
  MTYPECODE_UINT8,
  MTYPECODE_INT32,
  MTYPECODE_NVFP4,
  MTYPECODE_MXFP4,
  MTYPECODE_FP8E5M2,
  MTYPECODE_FP8E4M3,
  MTYPECODE_FP16,
  MTYPECODE_BF16,
  MTYPECODE_TF32,
  MTYPECODE_FP32,
  MTYPECODE_FP2PACK4,
  MTYPECODE_FP2PACK5
};

typedef union {
  struct {
    uint64_t type_code :  4;
    uint64_t table_set :  4;
    uint64_t pad       : 24;
  };
  uint64_t val;
} mcfg_t;

static inline int check_mtreg_num(int num) {
  assert(num >= 0 && num < 4);
  return num;
}

static inline int check_mareg_num(int num) {
  assert(num >= 4 && num < 8);
  return num - 4;
}

static inline int check_mtreg_row(int row) {
  assert(row >= 0 && row < ROWNUM);
  return row;
}

static inline int check_macc_row(int row) {
  assert(row >= 0 && row < ROWNUM);
  return row;
}

static inline int check_mtreg_idx(int idx, int elen) {
  assert(idx >= 0 && idx < TRLEN/elen);
  return idx;
}

static inline int check_macc_idx(int idx, int elen) {
  assert(idx >= 0 && idx < ARLEN/elen);
  return idx;
}

static inline int check_mtok_idx(int idx) {
  assert(idx >= 0 && idx < MSYNC);
  return idx;
}

#define mtreg_l64(num, row, idx) (cpu.mtr[check_mtreg_num(num)][check_mtreg_row(row)]._64[check_mtreg_idx(idx, 64)])
#define mtreg_l32(num, row, idx) (cpu.mtr[check_mtreg_num(num)][check_mtreg_row(row)]._32[check_mtreg_idx(idx, 32)])
#define mtreg_l16(num, row, idx) (cpu.mtr[check_mtreg_num(num)][check_mtreg_row(row)]._16[check_mtreg_idx(idx, 16)])
#define mtreg_l8(num, row, idx)  (cpu.mtr[check_mtreg_num(num)][check_mtreg_row(row)]._8[check_mtreg_idx(idx, 8)])
#define macc_l64(num, row, idx)  (cpu.macc[check_mareg_num(num)][check_macc_row(row)]._64[check_macc_idx(idx, 64)])
#define macc_l32(num, row, idx)  (cpu.macc[check_mareg_num(num)][check_macc_row(row)]._32[check_macc_idx(idx, 32)])
#define macc_l16(num, row, idx)  (cpu.macc[check_mareg_num(num)][check_macc_row(row)]._16[check_macc_idx(idx, 16)])
#define macc_l8(num, row, idx)   (cpu.macc[check_mareg_num(num)][check_macc_row(row)]._8[check_macc_idx(idx, 8)])

void set_mreg(int mtr_num, int mtr_row, int mtr_idx, rtlreg_t src_data, uint64_t msew);
void get_mreg(int mtr_num, int mtr_row, int mtr_idx, rtlreg_t *dst, uint64_t msew, bool is_signed);

#endif //__RISCV64_MREG_H__

#endif // CONFIG_RV_AME
