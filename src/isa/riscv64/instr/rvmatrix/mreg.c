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

#ifdef CONFIG_RVMATRIX

#include "mreg.h"
#include <stdio.h>
#include "isa.h"


void set_mreg(bool isacc, int mtr_num, int mtr_row, int mtr_idx, rtlreg_t src_data, uint64_t msew) {
  Assert(msew <= 3, "msew >= 4 is reserved\n");
  switch (msew) {
    case 0 : src_data = src_data & 0xff; break;
    case 1 : src_data = src_data & 0xffff; break;
    case 2 : src_data = src_data & 0xffffffff; break;
    case 3 : src_data = src_data & 0xffffffffffffffff; break;
  }
  if (isacc){
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

void get_mreg(bool isacc, int mtr_num, int mtr_row, int mtr_idx, rtlreg_t *dst, uint64_t msew, bool is_signed) {
  Assert(msew <= 3, "msew >= 4 is reserved\n");
  // TODO: check index size not larger than mtreg size here
  if (isacc) {
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

#endif // CONFIG_RVMATRIX