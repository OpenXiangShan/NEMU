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

#ifdef CONFIG_RV_AME

#include "mreg.h"
#ifdef CONFIG_AME_TILEREG_UB_CHECK
#include <stdio.h>
#endif
#include "isa.h"

_Static_assert(CONFIG_RV_AME_TLEN % CONFIG_RV_AME_TRLEN == 0,
               "TLEN must be divisible by TRLEN");
_Static_assert(CONFIG_RV_AME_TRLEN % 64 == 0,
               "TRLEN must be a multiple of 64");
_Static_assert((CONFIG_RV_AME_TLEN / CONFIG_RV_AME_TRLEN) * CONFIG_RV_AME_MLEN % 64 == 0,
               "ARLEN must be a multiple of 64");
_Static_assert(CONFIG_RV_AME_MSYNC == 8 ||
               CONFIG_RV_AME_MSYNC == 16 ||
               CONFIG_RV_AME_MSYNC == 32,
               "MSYNC must be one of 8/16/32");

#ifdef CONFIG_AME_TILEREG_UB_CHECK
// All current matrix writers produce a rectangle starting at (0, 0). Track
// the latest write, not the union of historical writes: consumers must not
// depend on the preserved values outside a partial load or MMA result.
typedef struct {
  uint64_t rows;
  // Column extent in bytes. Keeping byte units makes mcfg changes explicit.
  uint64_t columns;
  uint64_t pc;
} matrix_write_region_t;

static matrix_write_region_t matrix_write_regions[8];

void ame_matrix_region_mark_write(int mreg_num, uint64_t rows, uint64_t columns,
                                 uint64_t pc) {
  if (rows == 0 || columns == 0) return;
  Assert(mreg_num >= 0 && mreg_num < 8, "Invalid matrix register m%d", mreg_num);
  Assert(rows <= ROWNUM && columns <= (mreg_num < 4 ? TRENUM8 : ARENUM8),
         "Invalid matrix write region");
  matrix_write_regions[mreg_num] = (matrix_write_region_t) {rows, columns, pc};
}

void ame_matrix_region_check_read(int mreg_num, uint64_t rows, uint64_t columns,
                                 uint64_t pc) {
  if (rows == 0 || columns == 0) return;
  Assert(mreg_num >= 0 && mreg_num < 8, "Invalid matrix register m%d", mreg_num);
  const matrix_write_region_t *last = &matrix_write_regions[mreg_num];
  if (rows <= last->rows && columns <= last->columns) return;

  fprintf(stderr, "UB: matrix read at pc=0x%016lx reads m%d outside its latest write region; "
          "read=%lu rows x %lu bytes, valid=%lu rows x %lu bytes",
          (unsigned long)pc, mreg_num, (unsigned long)rows,
          (unsigned long)columns, (unsigned long)last->rows, (unsigned long)last->columns);
  if (last->rows != 0 && last->columns != 0) {
    fprintf(stderr, "; last write at pc=0x%016lx\n", (unsigned long)last->pc);
  } else {
    fprintf(stderr, "; no prior matrix write\n");
  }
}

#endif

uint8_t *get_mreg_row_addr(int mtr_num, uint64_t mtr_row) {
  if (mtr_num >= 4) {
    return cpu.macc[check_mareg_num(mtr_num)][check_macc_row(mtr_row)]._8;
  }
  return cpu.mtr[check_mtreg_num(mtr_num)][check_mtreg_row(mtr_row)]._8;
}

#ifdef CONFIG_RV_AME_FP4
uint8_t get_mreg_nibble(int mtr_num, uint64_t mtr_row, uint64_t mtr_idx) {
  uint8_t byte = mtreg_l8(mtr_num, mtr_row, mtr_idx >> 1);
  return raw_fp4_get_nibble(byte, mtr_idx);
}

void set_mreg_nibble(int mtr_num, uint64_t mtr_row, uint64_t mtr_idx,
                     uint8_t src_data) {
  uint8_t *byte = &mtreg_l8(mtr_num, mtr_row, mtr_idx >> 1);
  *byte = raw_fp4_set_nibble(*byte, mtr_idx, src_data);
}
#endif

void set_mreg(int mtr_num, uint64_t mtr_row, uint64_t mtr_idx,
              rtlreg_t src_data, uint64_t msew) {
  Assert(msew <= 3, "msew >= 4 is reserved\n");
  switch (msew) {
    case 0 : src_data = src_data & 0xff; break;
    case 1 : src_data = src_data & 0xffff; break;
    case 2 : src_data = src_data & 0xffffffff; break;
    case 3 : src_data = src_data & 0xffffffffffffffff; break;
  }
  if (mtr_num >= 4){
    switch (msew) {
      case 0 : macc_l8(mtr_num, mtr_row, mtr_idx)  = (uint8_t )src_data; break;
      case 1 : macc_l16(mtr_num, mtr_row, mtr_idx) = (uint16_t)src_data; break;
      case 2 : macc_l32(mtr_num, mtr_row, mtr_idx) = (uint32_t)src_data; break;
      case 3 : macc_l64(mtr_num, mtr_row, mtr_idx) = (uint64_t)src_data; break;
    }
  } else {
    switch (msew) {
      case 0 : mtreg_l8(mtr_num, mtr_row, mtr_idx)  = (uint8_t )src_data; break;
      case 1 : mtreg_l16(mtr_num, mtr_row, mtr_idx) = (uint16_t)src_data; break;
      case 2 : mtreg_l32(mtr_num, mtr_row, mtr_idx) = (uint32_t)src_data; break;
      case 3 : mtreg_l64(mtr_num, mtr_row, mtr_idx) = (uint64_t)src_data; break;
    }
  }
}

void get_mreg(int mtr_num, uint64_t mtr_row, uint64_t mtr_idx,
              rtlreg_t *dst, uint64_t msew, bool is_signed) {
  Assert(msew <= 3, "msew >= 4 is reserved\n");
  if (mtr_num >= 4) {
    switch (msew) {
      case 0 : *dst = is_signed ? (int64_t)(int8_t )macc_l8(mtr_num, mtr_row, mtr_idx)  : macc_l8(mtr_num, mtr_row, mtr_idx) ; break;
      case 1 : *dst = is_signed ? (int64_t)(int16_t)macc_l16(mtr_num, mtr_row, mtr_idx) : macc_l16(mtr_num, mtr_row, mtr_idx); break;
      case 2 : *dst = is_signed ? (int64_t)(int32_t)macc_l32(mtr_num, mtr_row, mtr_idx) : macc_l32(mtr_num, mtr_row, mtr_idx); break;
      case 3 : *dst = is_signed ? (int64_t)(int64_t)macc_l64(mtr_num, mtr_row, mtr_idx) : macc_l64(mtr_num, mtr_row, mtr_idx); break;
    }
  } else {
    switch (msew) {
      case 0 : *dst = is_signed ? (int64_t)(int8_t )mtreg_l8(mtr_num, mtr_row, mtr_idx)  : mtreg_l8(mtr_num, mtr_row, mtr_idx) ; break;
      case 1 : *dst = is_signed ? (int64_t)(int16_t)mtreg_l16(mtr_num, mtr_row, mtr_idx) : mtreg_l16(mtr_num, mtr_row, mtr_idx); break;
      case 2 : *dst = is_signed ? (int64_t)(int32_t)mtreg_l32(mtr_num, mtr_row, mtr_idx) : mtreg_l32(mtr_num, mtr_row, mtr_idx); break;
      case 3 : *dst = is_signed ? (int64_t)(int64_t)mtreg_l64(mtr_num, mtr_row, mtr_idx) : mtreg_l64(mtr_num, mtr_row, mtr_idx); break;
    }
  }
}

#endif // CONFIG_RV_AME
