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

#ifndef __RISCV64_MLDST_TRANSPOSE_FAST_H__
#define __RISCV64_MLDST_TRANSPOSE_FAST_H__

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#if defined(CONFIG_RV_AME) && defined(CONFIG_AME_MLDST_VECTORIZE) && \
    defined(__x86_64__) && (defined(__GNUC__) || defined(__clang__))

#define AME_MLDST_HAS_TRANSPOSE_FAST_BACKEND

bool ame_transpose_available(void);

bool ame_transpose_matrix(
  const uint8_t *src, size_t src_row_stride,
  uint8_t *dst, size_t dst_row_stride,
  size_t rows, size_t columns, int msew
);

#endif

#endif // __RISCV64_MLDST_TRANSPOSE_FAST_H__
