/***************************************************************************************
* Copyright (c) 2020-2025 Institute of Computing Technology, Chinese Academy of Sciences
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
#include <stdint.h>

#ifdef CONFIG_RVMATRIX

#include <limits.h>

#include "mcompute_impl.h"
#include <cpu/cpu.h>
#include "mcommon.h"

void require_matrix() {
  // Matrix-state checks are intentionally disabled in this NEMU branch.
}

void matrix_compute_integer(uint8_t destination, uint8_t source_a,
                            uint8_t source_b, uint16_t m, uint16_t n,
                            uint16_t k, uint8_t destination_size,
                            uint8_t source_a_size, uint8_t source_b_size,
                            bool source_a_signed, bool source_b_signed,
                            bool saturate) {
  const unsigned destination_bits = 8u << destination_size;
  const int64_t maximum = destination_bits == 64
                              ? INT64_MAX
                              : (INT64_C(1) << (destination_bits - 1)) - 1;
  const int64_t minimum = destination_bits == 64
                              ? INT64_MIN
                              : -(INT64_C(1) << (destination_bits - 1));

  for (uint16_t row = 0; row < m; ++row) {
    for (uint16_t column = 0; column < n; ++column) {
      for (uint16_t reduction = 0; reduction < k; ++reduction) {
        rtlreg_t accumulator = 0;
        rtlreg_t lhs = 0;
        rtlreg_t rhs = 0;
        get_mreg(source_a, row, reduction, &lhs, source_a_size,
                 source_a_signed);
        get_mreg(source_b, column, reduction, &rhs, source_b_size,
                 source_b_signed);
        get_mreg(destination, row, column, &accumulator, destination_size,
                 true);

        if (saturate) {
          int64_t result = (int64_t)lhs * (int64_t)rhs +
                           (int64_t)accumulator;
          if (result > maximum) result = maximum;
          if (result < minimum) result = minimum;
          set_mreg(destination, row, column, (rtlreg_t)result,
                   destination_size);
        } else {
          accumulator = lhs * rhs + accumulator;
          set_mreg(destination, row, column, accumulator, destination_size);
        }
      }
    }
  }
}

#endif // CONFIG_RVMATRIX
