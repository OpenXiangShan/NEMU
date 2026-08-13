/***************************************************************************************
* Copyright (c) 2026 Institute of Computing Technology, Chinese Academy of Sciences
*
* NEMU is licensed under Mulan PSL v2.
* You can use this software according to the terms and conditions of the Mulan PSL v2.
* You may obtain a copy of the license at:
*          http://license.coscl.org.cn/MulanPSL2
***************************************************************************************/

#ifndef __AME_RAW_FP4_H__
#define __AME_RAW_FP4_H__

#include <common.h>

#ifdef __cplusplus
extern "C" {
#endif

static inline size_t raw_fp4_packed_bytes(size_t logical_elements) {
  return (logical_elements + 1) / 2;
}

static inline int raw_fp4_memory_rows(int rows, int columns, bool transpose) {
  return transpose ? columns : rows;
}

static inline int raw_fp4_memory_columns(int rows, int columns, bool transpose) {
  return transpose ? rows : columns;
}

static inline int raw_fp4_register_row(int memory_row, int memory_column, bool transpose) {
  return transpose ? memory_column : memory_row;
}

static inline int raw_fp4_register_column(int memory_row, int memory_column, bool transpose) {
  return transpose ? memory_row : memory_column;
}

static inline uint8_t raw_fp4_get_nibble(uint8_t packed_byte, int logical_index) {
  return (packed_byte >> ((logical_index & 1) << 2)) & 0xf;
}

static inline uint8_t raw_fp4_set_nibble(uint8_t packed_byte, int logical_index,
                                          uint8_t nibble) {
  uint8_t shift = (logical_index & 1) << 2;
  return (packed_byte & ~(0xf << shift)) | ((nibble & 0xf) << shift);
}

#if defined(CONFIG_RV_AME) && defined(CONFIG_RV_AME_FP4)
struct Decode;
bool raw_fp4_matrix_access(struct Decode *s, vaddr_t base, vaddr_t stride,
                           int row, int column, bool transpose, bool store, int mreg_id);
#endif

#ifdef __cplusplus
}
#endif

#endif // __AME_RAW_FP4_H__
