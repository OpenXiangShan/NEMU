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

#include "amuhash_math.h"

#if defined(CONFIG_RV_AME) && defined(CONFIG_SHARE_REF)

static uint64_t hash_scalar(const uint8_t *data, size_t stride, size_t rows, size_t bytes) {
  uint64_t hash = 0;
  const size_t gap = stride - bytes;
  const bool jump = gap >= HASH_PORTABLE_MIN_GAP_BYTES && rows > 1;
  gf32_multiplier_t factor;
  if (jump) gf32_prepare(&factor, hash_shift_factor(gap));
  for (size_t row = rows; row-- > 0;) {
    if (row + 1 != rows) hash = jump ? gf32_multiply_prepared(hash, &factor) :
                                     hash_advance_unwritten(hash, gap);
    hash = hash_update_data(hash, data + row * stride, bytes);
  }
  return hash;
}

AMU_HASH_TARGET_BASELINE
uint64_t hash_amu_portable(const uint8_t *data, size_t stride, size_t rows, size_t bytes) {
  if (rows < HASH_LANES || stride < 8 || bytes == 0 ||
      rows * stride < HASH_PARALLEL_MIN_REGION_BYTES)
    return hash_scalar(data, stride, rows, bytes);
  const size_t rows_per_lane = rows / HASH_LANES;
  const size_t span = rows_per_lane * stride;
  const size_t gap = stride - bytes;
  const bool jump = gap >= HASH_PORTABLE_MIN_GAP_BYTES && rows_per_lane > 1;
  gf32_multiplier_t gap_factor;
  if (jump) gf32_prepare(&gap_factor, hash_shift_factor(gap));
  uint64_t lanes[HASH_LANES] = {0};
  for (size_t row = rows_per_lane; row-- > 0;) {
    if (row + 1 != rows_per_lane) {
      if (jump) {
        for (unsigned lane = 0; lane < HASH_LANES; ++lane)
          lanes[lane] = gf32_multiply_prepared(lanes[lane], &gap_factor);
      } else {
        size_t left = gap;
        while (left >= 7) {
          for (unsigned lane = 0; lane < HASH_LANES; ++lane) lanes[lane] = gf32_shift(lanes[lane], 63);
          left -= 7;
        }
        while (left--)
          for (unsigned lane = 0; lane < HASH_LANES; ++lane) lanes[lane] = gf32_shift(lanes[lane], 9);
      }
    }
    size_t left = bytes;
    while (left >= 7) {
      left -= 7;
      for (unsigned lane = 0; lane < HASH_LANES; ++lane) {
        const uint8_t *p = data + lane * span + row * stride;
        uint64_t symbols;
        if (left) symbols = hash_expand_seven(hash_load_le64(p + left - 1) >> 8);
        else if (bytes >= 8) symbols = hash_expand_seven(hash_load_le64(p));
        else symbols = hash_pack_seven(p);
        lanes[lane] = gf32_shift(lanes[lane], 63) ^ symbols;
      }
    }
    while (left) {
      --left;
      for (unsigned lane = 0; lane < HASH_LANES; ++lane)
        lanes[lane] = gf32_shift(lanes[lane], 9) ^ UINT64_C(0x100) ^ data[lane * span + row * stride + left];
    }
  }
  gf32_multiplier_t factor;
  gf32_prepare(&factor, hash_shift_factor(span));
  const size_t tail = rows % HASH_LANES;
  uint64_t hash;
  unsigned remaining;
  if (tail) {
    hash = hash_scalar(data + HASH_LANES * span, stride, tail, bytes);
    remaining = HASH_LANES;
  } else {
    hash = lanes[HASH_LANES - 1];
    remaining = HASH_LANES - 1;
  }
  while (remaining) hash = gf32_multiply_prepared(hash, &factor) ^ lanes[--remaining];
  return hash;
}

#endif
