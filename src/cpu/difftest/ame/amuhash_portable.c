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

#include "amuhash.h"

#if defined(CONFIG_RV_AME) && defined(CONFIG_SHARE_REF)

#include <string.h>

#include "amuhash_math.h"

// Keep the independent lane loop intact until the vectorizer has seen it.
#if defined(__clang__)
#define AMU_HASH_LANE_LOOP _Pragma("clang loop unroll(disable)")
#elif defined(__GNUC__) && __GNUC__ >= 8
#define AMU_HASH_LANE_LOOP _Pragma("GCC unroll 0")
#else
#define AMU_HASH_LANE_LOOP
#endif

// Precompute the sixteen polynomial multiples needed for four-bit lookup steps.
static inline void gf128_prepare_multiplier(gf128_multiplier_t *table, gf128_t factor) {
  table->multiples[0] = (gf128_t){0};
  table->multiples[1] = factor;
  for (unsigned bit = 1; bit < 4; ++bit) {
    const unsigned base = 1U << bit;
    const gf128_t previous = table->multiples[base >> 1];
    table->multiples[base] = (gf128_t){
      .lo = (previous.lo << 1) ^ ((UINT64_C(0) - (previous.hi >> 63)) & UINT64_C(0x87)),
      .hi = (previous.hi << 1) | (previous.lo >> 63),
    };
    for (unsigned i = 1; i < base; ++i) {
      table->multiples[base + i] = (gf128_t){
        .lo = table->multiples[base].lo ^ table->multiples[i].lo,
        .hi = table->multiples[base].hi ^ table->multiples[i].hi,
      };
    }
  }
}

static inline gf128_t gf128_multiply_prepared(gf128_t value, const gf128_multiplier_t *table) {
  gf128_t result = {0};
  for (unsigned half = 0; half < 2; ++half) {
    uint64_t bits = half == 0 ? value.hi : value.lo;
    for (unsigned nibble = 0; nibble < 16; ++nibble) {
      gf128_shift(&result, 4);
      const gf128_t multiple = table->multiples[bits >> 60];
      result.lo ^= multiple.lo;
      result.hi ^= multiple.hi;
      bits <<= 4;
    }
  }
  return result;
}

static inline gf128_t gf128_multiply(gf128_t a, gf128_t b) {
  gf128_multiplier_t table;
  gf128_prepare_multiplier(&table, b);
  return gf128_multiply_prepared(a, &table);
}
// x^(9 * bytes) modulo the polynomial, used to advance over logical byte positions.
static inline gf128_t hash_shift_factor(size_t bytes) {
  gf128_t factor = {.lo = 1};
  bool first = true;
  for (size_t bit = 0; bytes != 0; ++bit, bytes >>= 1) {
    if (bytes & 1) {
      factor = first ? hash_shift_powers[bit] :
                       gf128_multiply(factor, hash_shift_powers[bit]);
      first = false;
    }
  }
  return factor;
}
// Use a prepared multiplier for large gaps; otherwise advance over zero symbols.
static inline gf128_t hash_scalar_rows(const uint8_t *reg_data, size_t reg_row_bytes,
                                       size_t active_rows, size_t active_row_bytes,
                                       const gf128_multiplier_t *gap_table) {
  gf128_t hash = {0};
  if (active_rows == 0 || active_row_bytes == 0) return hash;
  const size_t gap_bytes = reg_row_bytes - active_row_bytes;
  for (size_t row = active_rows; row-- > 0;) {
    // The highest row's unwritten bytes precede a zero remainder.
    if (row + 1 != active_rows) {
      if (gap_table != NULL) hash = gf128_multiply_prepared(hash, gap_table);
      else hash_advance_unwritten(&hash, gap_bytes);
    }
    hash_update_data(&hash, reg_data + row * reg_row_bytes, active_row_bytes);
  }
  return hash;
}

AMU_HASH_TARGET_BASELINE
static gf128_t hash_scalar(const uint8_t *reg_data, size_t reg_row_bytes,
                          size_t active_rows, size_t active_row_bytes) {
  return hash_scalar_rows(reg_data, reg_row_bytes, active_rows, active_row_bytes, NULL);
}

static gf128_t hash_scalar_shift(const uint8_t *reg_data, size_t reg_row_bytes,
                                size_t active_rows, size_t active_row_bytes) {
  if (active_rows == 0 || active_row_bytes == 0) return (gf128_t){0};
  const size_t gap_bytes = reg_row_bytes - active_row_bytes;
  gf128_multiplier_t gap;
  gf128_prepare_multiplier(&gap, hash_shift_factor(gap_bytes));
  return hash_scalar_rows(reg_data, reg_row_bytes, active_rows, active_row_bytes, &gap);
}

static inline uint64_t hash_load_le64(const uint8_t *data) {
#if defined(__BYTE_ORDER__) && __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
  uint64_t value;
  memcpy(&value, data, sizeof(value));
  return value;
#else
  return (uint64_t)data[0] | ((uint64_t)data[1] << 8) | ((uint64_t)data[2] << 16) |
         ((uint64_t)data[3] << 24) | ((uint64_t)data[4] << 32) | ((uint64_t)data[5] << 40) |
         ((uint64_t)data[6] << 48) | ((uint64_t)data[7] << 56);
#endif
}

static inline uint64_t hash_pack_seven_fast(const uint8_t *row, size_t byte, size_t reg_row_bytes) {
  // The eight-byte load must remain within the row; its last byte is ignored.
  if (reg_row_bytes - byte < 8) return hash_pack_seven(row + byte);
  uint64_t packed = hash_load_le64(row + byte) & UINT64_C(0x00ffffffffffffff);
  // Insert one zero between adjacent bytes, then set the seven coverage bits.
  packed = (packed & UINT64_C(0xffffffff)) | ((packed >> 32) << 36);
  uint64_t moved = packed & UINT64_C(0x0ff00000ffff0000);
  packed = (packed & UINT64_C(0x000ffff00000ffff)) | (moved << 2);
  moved = packed & UINT64_C(0x000ff003fc00ff00);
  return (packed & ~UINT64_C(0x000ff003fc00ff00)) | (moved << 1) |
         UINT64_C(0x4020100804020100);
}

static inline void hash_update_byte_lanes(hash_lanes_t *hash, const uint64_t symbols[HASH_LANES]) {
  AMU_HASH_LANE_LOOP
  for (size_t lane = 0; lane < HASH_LANES; ++lane) {
    const uint64_t high = hash->hi[lane] >> 55;
    hash->hi[lane] = (hash->hi[lane] << 9) | (hash->lo[lane] >> 55);
    hash->lo[lane] = (hash->lo[lane] << 9) ^ gf128_fold_word(high) ^ symbols[lane];
  }
}

static inline void hash_update_seven_lanes(hash_lanes_t *hash, const uint64_t symbols[HASH_LANES]) {
  AMU_HASH_LANE_LOOP
  for (size_t lane = 0; lane < HASH_LANES; ++lane) {
    const uint64_t high = hash->hi[lane] >> 1;
    hash->hi[lane] = ((hash->hi[lane] << 63) | (hash->lo[lane] >> 1)) ^
                     (high >> 62) ^ (high >> 57);
    hash->lo[lane] = (hash->lo[lane] << 63) ^ gf128_fold_word(high) ^ symbols[lane];
  }
}

static inline void hash_multiply_lanes(hash_lanes_t *hash,
                                       const gf128_multiplier_t *factor) {
  for (size_t lane = 0; lane < HASH_LANES; ++lane) {
    const gf128_t value = {.lo = hash->lo[lane], .hi = hash->hi[lane]};
    const gf128_t shifted = gf128_multiply_prepared(value, factor);
    hash->lo[lane] = shifted.lo;
    hash->hi[lane] = shifted.hi;
  }
}

AMU_HASH_TARGET_BASELINE
static gf128_t hash_parallel_portable(const uint8_t *reg_data, size_t reg_row_bytes,
                                     size_t active_rows, size_t active_row_bytes) {
  if (active_rows < HASH_LANES || reg_row_bytes < 8 || active_row_bytes == 0)
    return hash_scalar(reg_data, reg_row_bytes, active_rows, active_row_bytes);
  const size_t rows_per_lane = active_rows / HASH_LANES;
  const size_t lane_span_bytes = rows_per_lane * reg_row_bytes;
  const size_t gap_bytes = reg_row_bytes - active_row_bytes;
  const bool jump_gap = gap_bytes >= HASH_PORTABLE_MIN_GAP_BYTES && rows_per_lane > 1;
  gf128_multiplier_t gap;
  if (jump_gap) gf128_prepare_multiplier(&gap, hash_shift_factor(gap_bytes));
  hash_lanes_t lanes = {0};
  const uint64_t zeros[HASH_LANES] = {0};
  for (size_t row = rows_per_lane; row-- > 0;) {
    if (row + 1 != rows_per_lane) {
      if (jump_gap) hash_multiply_lanes(&lanes, &gap);
      else {
        size_t bytes = gap_bytes;
        while (bytes >= HASH_BATCH_BYTES) {
          hash_update_seven_lanes(&lanes, zeros);
          bytes -= HASH_BATCH_BYTES;
        }
        while (bytes-- != 0) hash_update_byte_lanes(&lanes, zeros);
      }
    }
    const uint8_t *data = reg_data + row * reg_row_bytes;
    size_t byte = active_row_bytes;
    while (byte >= HASH_BATCH_BYTES) {
      byte -= HASH_BATCH_BYTES;
      uint64_t symbols[HASH_LANES];
      for (size_t lane = 0; lane < HASH_LANES; ++lane)
        symbols[lane] = hash_pack_seven_fast(data + lane * lane_span_bytes, byte, reg_row_bytes);
      hash_update_seven_lanes(&lanes, symbols);
    }
    while (byte != 0) {
      --byte;
      uint64_t symbols[HASH_LANES];
      for (size_t lane = 0; lane < HASH_LANES; ++lane)
        symbols[lane] = UINT64_C(0x100) | data[lane * lane_span_bytes + byte];
      hash_update_byte_lanes(&lanes, symbols);
    }
  }
  gf128_multiplier_t lane_factor;
  gf128_prepare_multiplier(&lane_factor, hash_shift_factor(lane_span_bytes));
  const size_t tail_rows = active_rows % HASH_LANES;
  gf128_t hash;
  size_t remaining_lanes = HASH_LANES;
  if (tail_rows != 0) {
    hash = hash_scalar(reg_data + HASH_LANES * lane_span_bytes, reg_row_bytes,
                       tail_rows, active_row_bytes);
  } else {
    remaining_lanes = HASH_LANES - 1;
    hash = (gf128_t){.lo = lanes.lo[HASH_LANES - 1], .hi = lanes.hi[HASH_LANES - 1]};
  }
  // Concatenate the highest group through lane zero, advancing by full row strides.
  while (remaining_lanes != 0) {
    const size_t lane = --remaining_lanes;
    hash = gf128_multiply_prepared(hash, &lane_factor);
    hash.lo ^= lanes.lo[lane];
    hash.hi ^= lanes.hi[lane];
  }
  return hash;
}


static inline bool hash_can_parallelize(size_t reg_row_bytes, size_t active_rows) {
  return reg_row_bytes >= 8 && active_rows >= HASH_LANES &&
         active_rows * reg_row_bytes >= HASH_PARALLEL_MIN_REGION_BYTES;
}

AMU_HASH_TARGET_BASELINE
gf128_t hash_amu_portable(const uint8_t *reg_data, size_t reg_row_bytes,
                          size_t active_rows, size_t active_row_bytes) {
  const size_t gap_bytes = reg_row_bytes - active_row_bytes;
  const bool small_tile = active_rows < HASH_SMALL_SPARSE_ROW_LIMIT &&
                          active_row_bytes <= HASH_SMALL_SPARSE_MAX_ACTIVE_ROW_BYTES;
  const bool small_sparse = small_tile && gap_bytes >= HASH_PORTABLE_MIN_GAP_BYTES;
  if (!small_sparse && hash_can_parallelize(reg_row_bytes, active_rows))
    return hash_parallel_portable(reg_data, reg_row_bytes, active_rows, active_row_bytes);
  if (gap_bytes >= HASH_PORTABLE_MIN_GAP_BYTES && active_rows > 1)
    return hash_scalar_shift(reg_data, reg_row_bytes, active_rows, active_row_bytes);
  return hash_scalar(reg_data, reg_row_bytes, active_rows, active_row_bytes);
}

#endif // defined(CONFIG_RV_AME) && defined(CONFIG_SHARE_REF)
