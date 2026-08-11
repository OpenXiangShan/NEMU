/***************************************************************************************
* Copyright (c) 2014-2021 Zihao Yu, Nanjing University
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

#ifndef __MEMORY_HOST_H__
#define __MEMORY_HOST_H__

#include <common.h>
#include "paddr.h"

static inline word_t host_read(void *addr, int len) {
  switch (len) {
    case 1:
    Logm("load: addr = %p, len = %d, data = 0x%x", addr, len, *(uint8_t  *)addr);
    return *(uint8_t  *)addr;
    case 2:
    Logm("load: addr = %p, len = %d, data = 0x%x", addr, len, *(uint16_t  *)addr);
    return *(uint16_t *)addr;
    case 4:
    Logm("load: addr = %p, len = %d, data = 0x%x", addr, len, *(uint32_t  *)addr);
    return *(uint32_t *)addr;
#ifdef CONFIG_ISA64
    case 8:
    Logm("load: addr = %p, len = %d, data = 0x%lx", addr, len, *(uint64_t  *)addr);
    return *(uint64_t *)addr;
#endif
    default: MUXDEF(CONFIG_RT_CHECK, assert(0), return 0);
  }
}

#ifdef CONFIG_RVMATRIX
#ifndef MATRIX_MSEW_FP2PACK4
#define MATRIX_MSEW_FP2PACK4 4
#endif

static inline bool host_matrix_is_fp2pack4(int msew) {
  return msew == MATRIX_MSEW_FP2PACK4;
}

static inline int host_matrix_width_bytes(int msew) {
  return host_matrix_is_fp2pack4(msew) ? 1 : (1 << msew);
}

static inline void host_read_matrix_fp2pack4(paddr_t pbase, paddr_t stride, int row,
                                             int column, bool transpose,
                                             char m_name, int mreg_id) {
  Assert(!transpose, "fp2pack4 transpose load is not supported");
  Assert(m_name == 'a', "fp2pack4 load is only valid for matrix A");
  Assert(mreg_id < 4, "fp2pack4 load destination must be a tile matrix register");

  const int row_reg_bytes = CONFIG_RVMATRIX_TRLEN / 8;
  const int group_reg_bytes = row_reg_bytes / 2;
  const int group_packed_bytes = group_reg_bytes / 4;
  const int group_elements = group_packed_bytes * 4;
  const int groups = (column + group_elements - 1) / group_elements;

  Assert(group_reg_bytes > 0 && group_packed_bytes > 0, "invalid fp2pack4 matrix packing");
  Assert(groups * group_reg_bytes <= row_reg_bytes, "fp2pack4 matrix row is too wide");

  for (int r = 0; r < row; r++) {
    for (int g = 0; g < groups; g++) {
      const int group_reg_base = g * group_reg_bytes;
      for (int b = 0; b < group_reg_bytes; b++) {
        set_mreg(mreg_id, r, group_reg_base + b, 0, 0);
      }

      int remaining = column - g * group_elements;
      if (remaining > group_elements) {
        remaining = group_elements;
      }
      const int packed_bytes = (remaining + 3) / 4;
      for (int b = 0; b < packed_bytes; b++) {
        word_t tmp = host_read(guest_to_host(pbase + g * group_packed_bytes + b), 1);
        set_mreg(mreg_id, r, group_reg_base + b, tmp, 0);
      }
    }
    pbase += stride;
  }
}

static inline void host_read_matrix(paddr_t pbase, paddr_t stride, int row,
                              int column, int msew, bool transpose,
                              char m_name, int mreg_id) {
  int width = host_matrix_width_bytes(msew);
  Logm("read matrix: base = %#lx, stride = %lu,\n"
       "             row = %d, column = %d, width = %d, transpose = %d, m_name = %c",
       pbase, stride, row, column, width, transpose, m_name);
  if (host_matrix_is_fp2pack4(msew)) {
    host_read_matrix_fp2pack4(pbase, stride, row, column, transpose, m_name, mreg_id);
    return;
  }
  int row_mem    = transpose ? column : row;
  int column_mem = transpose ? row : column;
  
  for (int r = 0; r < row_mem; r++) {
    for (int c = 0; c < column_mem; c++) {
      paddr_t addr = pbase + c * width;
      int r_reg = transpose ? c : r;
      int c_reg = transpose ? r : c;
      word_t tmp = host_read(guest_to_host(addr), width);
      set_mreg(mreg_id, r_reg, c_reg, tmp, msew);
    }
    pbase += stride;
  }
};
#endif // CONFIG_RVMATRIX

static inline void host_write(void *addr, int len, word_t data) {
  Logm("write: addr = %p, len = %d, data = 0x%lx", addr, len, data);
  switch (len) {
    case 1: *(uint8_t  *)addr = data; return;
    case 2: *(uint16_t *)addr = data; return;
    case 4: *(uint32_t *)addr = data; return;
    IFDEF(CONFIG_ISA64, case 8: *(uint64_t *)addr = data; return);
    IFDEF(CONFIG_RT_CHECK, default: assert(0));
  }
}

#ifdef CONFIG_RVMATRIX
static inline void host_write_matrix(paddr_t pbase, paddr_t stride, int row,
                              int column, int msew, bool transpose,
                              char m_name, int mreg_id) {
  Assert(!host_matrix_is_fp2pack4(msew), "fp2pack4 matrix store is not supported");
  int width = 1 << msew;
  Logm("write matrix: base = %#lx, stride = %lu,\n"
       "              row = %d, column = %d, width = %d, transpose = %d, m_name = %c",
       pbase, stride, row, column, width, transpose, m_name);
  int row_mem    = transpose ? column : row;
  int column_mem = transpose ? row : column;
  
  for (int r = 0; r < row_mem; r++) {
    for (int c = 0; c < column_mem; c++) {
      paddr_t addr = pbase + c * width;
      int r_reg = transpose ? c : r;
      int c_reg = transpose ? r : c;
      word_t tmp;
      get_mreg(mreg_id, r_reg, c_reg, &tmp, msew, false);
      host_write(guest_to_host(addr), width, tmp);
    }
    pbase += stride;
  }
};
#endif // CONFIG_RVMATRIX

#endif
