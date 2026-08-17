/***************************************************************************************
* Copyright (c) 2026 Beijing Institute of Open Source Chip (BOSC)
* Copyright (c) 2026 Institute of Computing Technology, Chinese Academy of Sciences
*
* NEMU is licensed under Mulan PSL v2.
* You can use this software according to the terms and conditions of the Mulan PSL v2.
* You may obtain a copy of Mulan PSL v2 at:
*          http://license.coscl.org.cn/MulanPSL2
*
* THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND,
* EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT,
* MERCHANTABILITY OR FITNESS FOR A PARTICULAR PURPOSE.
*
* See the Mulan PSL v2 for more details.
***************************************************************************************/

#include <common.h>

#if defined(CONFIG_RV_AME) && defined(CONFIG_AME_MLDST_VECTORIZE)

#include "mldst_fast.h"
#include "mldst_transpose_fast.h"
#include "mreg.h"

#if defined(__x86_64__) && (defined(__GNUC__) || defined(__clang__))
#define AME_TARGET_BASELINE __attribute__((target("arch=x86-64")))
#else
#define AME_TARGET_BASELINE
#endif

AME_TARGET_BASELINE
static bool get_element_size(int msew, size_t *element_size) {
  if ((unsigned)msew > 3) {
    return false;
  }
  *element_size = (size_t)1 << msew;
  return true;
}

AME_TARGET_BASELINE
static size_t get_mreg_row_stride(int mreg_id) {
  return mreg_id >= 4 ? ARENUM8 : TRENUM8;
}

AME_TARGET_BASELINE
static bool try_fast_transpose(
  const uint8_t *src, size_t src_row_stride,
  uint8_t *dst, size_t dst_row_stride,
  size_t rows, size_t columns, int msew
) {
#ifdef AME_MLDST_HAS_TRANSPOSE_FAST_BACKEND
  return ame_transpose_available() &&
         ame_transpose_matrix(src, src_row_stride,
                              dst, dst_row_stride,
                              rows, columns, msew);
#else
  (void)src;
  (void)src_row_stride;
  (void)dst;
  (void)dst_row_stride;
  (void)rows;
  (void)columns;
  (void)msew;
  return false;
#endif
}

AME_TARGET_BASELINE
bool try_fast_matrix_load(
  uint8_t *host_base, uint64_t stride,
  int row, int column, int msew, bool transpose, int mreg_id
) {
  size_t element_size;
  if (!get_element_size(msew, &element_size)) {
    return false;
  }

  size_t mreg_row_stride = get_mreg_row_stride(mreg_id);
  uint8_t *mreg_base = get_mreg_row_addr(mreg_id, 0);
  if (transpose) {
    return try_fast_transpose(
      host_base, (size_t)stride,
      mreg_base, mreg_row_stride,
      (size_t)column, (size_t)row, msew
    );
  } else { // !transpose
    size_t row_bytes = (size_t)column * element_size;
    for (int r = 0; r < row; r++) {
      __builtin_memcpy(mreg_base + (size_t)r * mreg_row_stride,
                      host_base + (size_t)r * (size_t)stride, row_bytes);
    }
  }
  return true;
}

AME_TARGET_BASELINE
bool try_fast_matrix_store(
  uint8_t *host_base, uint64_t stride,
  int row, int column, int msew, bool transpose, int mreg_id
) {
  size_t element_size;
  if (!get_element_size(msew, &element_size)) {
    return false;
  }

  size_t mreg_row_stride = get_mreg_row_stride(mreg_id);
  uint8_t *mreg_base = get_mreg_row_addr(mreg_id, 0);
  if (transpose) {
    if (stride < (uint64_t)row * element_size) {
      return false;
    }
    return try_fast_transpose(
      mreg_base, mreg_row_stride,
      host_base, (size_t)stride,
      (size_t)row, (size_t)column, msew
    );
  } else { // !transpose
    size_t row_bytes = (size_t)column * element_size;
    for (int r = 0; r < row; r++) {
      __builtin_memcpy(host_base + (size_t)r * (size_t)stride,
                      mreg_base + (size_t)r * mreg_row_stride, row_bytes);
    }
    return true;
  }
}

#undef AME_TARGET_BASELINE

#endif // CONFIG_RV_AME && CONFIG_AME_MLDST_VECTORIZE
