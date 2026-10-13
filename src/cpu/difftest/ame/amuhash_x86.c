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

#if defined(CONFIG_RV_AME) && defined(CONFIG_SHARE_REF) && defined(AMU_HASH_X86)

#ifdef __clang__
#include <immintrin.h>
#else
#pragma GCC push_options
#pragma GCC target("arch=x86-64")
#include <immintrin.h>
#pragma GCC pop_options
#endif

#include "amuhash_math.h"

static bool host_supports_fast_path;

AMU_HASH_TARGET_BASELINE
__attribute__((constructor))
static void detect_hash_host_features(void) {
  __builtin_cpu_init();
  host_supports_fast_path = __builtin_cpu_supports("avx2") &&
                            __builtin_cpu_supports("pclmul");
}

AMU_HASH_TARGET_AVX2
static inline gf128_t gf128_multiply_pclmul(gf128_t a, gf128_t b) {
  const __m128i av = _mm_set_epi64x(a.hi, a.lo);
  const __m128i bv = _mm_set_epi64x(b.hi, b.lo);
  const __m128i low_product = _mm_clmulepi64_si128(av, bv, 0x00);
  const __m128i high_product = _mm_clmulepi64_si128(av, bv, 0x11);
  const __m128i ax = _mm_xor_si128(av, _mm_srli_si128(av, 8));
  const __m128i bx = _mm_xor_si128(bv, _mm_srli_si128(bv, 8));
  const __m128i cross = _mm_xor_si128(_mm_clmulepi64_si128(ax, bx, 0x00),
                                    _mm_xor_si128(low_product, high_product));
  const __m128i low = _mm_xor_si128(low_product, _mm_slli_si128(cross, 8));
  const __m128i high = _mm_xor_si128(high_product, _mm_srli_si128(cross, 8));

  gf128_t result = {0}, upper = {0};
  _mm_storeu_si128((__m128i *)&result, low);
  _mm_storeu_si128((__m128i *)&upper, high);
  // Fold the upper half using x^128 = x^7+x^2+x+1, then fold its seven overflow bits.
  const uint64_t overflow = (upper.hi >> 63) ^ (upper.hi >> 62) ^ (upper.hi >> 57);
  result.lo ^= upper.lo ^ (upper.lo << 1) ^ (upper.lo << 2) ^ (upper.lo << 7) ^
               overflow ^ (overflow << 1) ^ (overflow << 2) ^ (overflow << 7);
  result.hi ^= upper.hi ^ (upper.hi << 1) ^ (upper.hi << 2) ^ (upper.hi << 7) ^
               (upper.lo >> 63) ^ (upper.lo >> 62) ^ (upper.lo >> 57);
  return result;
}

AMU_HASH_TARGET_AVX2
static gf128_t hash_shift_factor_avx2(size_t bytes) {
  gf128_t factor = {.lo = 1};
  for (size_t bit = 0; bytes != 0; ++bit, bytes >>= 1) {
    if (bytes & 1) {
      factor = gf128_multiply_pclmul(factor, hash_shift_powers[bit]);
    }
  }
  return factor;
}

AMU_HASH_TARGET_AVX2
static gf128_t hash_scalar_avx2(const uint8_t *reg_data, size_t reg_row_bytes,
                                size_t active_rows, size_t active_row_bytes) {
  gf128_t hash = {0};
  if (active_rows == 0 || active_row_bytes == 0) return hash;
  const size_t gap_bytes = reg_row_bytes - active_row_bytes;
  const bool jump_gap = gap_bytes >= HASH_AVX2_MIN_GAP_BYTES && active_rows > 1;
  const gf128_t gap_factor = jump_gap ? hash_shift_factor_avx2(gap_bytes) : (gf128_t){0};
  for (size_t row = active_rows; row-- > 0;) {
    if (row + 1 != active_rows) {
      if (jump_gap) hash = gf128_multiply_pclmul(hash, gap_factor);
      else hash_advance_unwritten(&hash, gap_bytes);
    }
    hash_update_data(&hash, reg_data + row * reg_row_bytes, active_row_bytes);
  }
  return hash;
}

// Each AVX2 lane holds one independent 128-bit hash, split between lo and hi.
AMU_HASH_TARGET_AVX2
static inline void hash_update_avx2(__m256i *lo, __m256i *hi,
                                    __m256i symbols, unsigned bits) {
  const __m256i high = _mm256_srli_epi64(*hi, 64 - bits);
  __m256i next_hi = _mm256_or_si256(_mm256_slli_epi64(*hi, bits),
                                  _mm256_srli_epi64(*lo, 64 - bits));
  // Only long shifts spill the polynomial reduction into the upper half.
  if (bits > 57) {
    next_hi = _mm256_xor_si256(next_hi, _mm256_xor_si256(
      _mm256_srli_epi64(high, 62), _mm256_srli_epi64(high, 57)));
  }
  __m256i next_lo = _mm256_xor_si256(_mm256_slli_epi64(*lo, bits), high);
  next_lo = _mm256_xor_si256(next_lo, _mm256_xor_si256(
    _mm256_slli_epi64(high, 1), _mm256_slli_epi64(high, 2)));
  *lo = _mm256_xor_si256(_mm256_xor_si256(next_lo, _mm256_slli_epi64(high, 7)), symbols);
  *hi = next_hi;
}

AMU_HASH_TARGET_AVX2
static inline void hash_update_byte_avx2(__m256i *lo, __m256i *hi,
                                         __m256i symbols) {
  hash_update_avx2(lo, hi, symbols, HASH_SYMBOL_BITS);
}

AMU_HASH_TARGET_AVX2
static inline void hash_update_seven_avx2(__m256i *lo, __m256i *hi,
                                          __m256i symbols) {
  hash_update_avx2(lo, hi, symbols, HASH_SYMBOL_BITS * HASH_BATCH_BYTES);
}

AMU_HASH_TARGET_AVX2
static inline __m256i hash_pack_seven_avx2(const uint8_t *data, __m256i offsets,
                                           size_t byte) {
  __m256i packed;
  // Read eight bytes wholly within each row. Drop the preceding byte when possible;
  // at byte zero, ignore the eighth byte. The caller requires a row width of at least eight.
  if (byte != 0) {
    packed = _mm256_i64gather_epi64((const long long *)(data + byte - 1), offsets, 1);
    packed = _mm256_srli_epi64(packed, 8);
  } else {
    packed = _mm256_i64gather_epi64((const long long *)data, offsets, 1);
    packed = _mm256_and_si256(packed, _mm256_set1_epi64x(UINT64_C(0x00ffffffffffffff)));
  }

  // Insert one zero between adjacent bytes, then set the seven coverage bits.
  const __m256i lower = _mm256_and_si256(packed, _mm256_set1_epi64x(UINT64_C(0xffffffff)));
  packed = _mm256_or_si256(lower, _mm256_slli_epi64(_mm256_srli_epi64(packed, 32), 36));
  __m256i moved = _mm256_and_si256(packed,
                                 _mm256_set1_epi64x(UINT64_C(0x0ff00000ffff0000)));
  packed = _mm256_or_si256(_mm256_and_si256(packed,
    _mm256_set1_epi64x(UINT64_C(0x000ffff00000ffff))), _mm256_slli_epi64(moved, 2));
  const __m256i odd_bytes = _mm256_set1_epi64x(UINT64_C(0x000ff003fc00ff00));
  moved = _mm256_and_si256(packed, odd_bytes);
  packed = _mm256_or_si256(_mm256_andnot_si256(odd_bytes, packed),
                          _mm256_slli_epi64(moved, 1));
  return _mm256_or_si256(packed, _mm256_set1_epi64x(UINT64_C(0x4020100804020100)));
}

AMU_HASH_TARGET_AVX2
static inline void hash_advance_unwritten_avx2(__m256i *lo, __m256i *hi,
                                               size_t bytes) {
  while (bytes >= HASH_BATCH_BYTES) {
    hash_update_seven_avx2(lo, hi, _mm256_setzero_si256());
    bytes -= HASH_BATCH_BYTES;
  }
  while (bytes-- != 0) hash_update_byte_avx2(lo, hi, _mm256_setzero_si256());
}

AMU_HASH_TARGET_AVX2
static inline void hash_multiply_lanes_avx2(__m256i *lo, __m256i *hi,
                                            gf128_t factor) {
  hash_lanes_t lanes;
  _mm256_storeu_si256((__m256i *)lanes.lo, *lo);
  _mm256_storeu_si256((__m256i *)lanes.hi, *hi);
  for (size_t lane = 0; lane < HASH_LANES; ++lane) {
    const gf128_t value = {.lo = lanes.lo[lane], .hi = lanes.hi[lane]};
    const gf128_t shifted = gf128_multiply_pclmul(value, factor);
    lanes.lo[lane] = shifted.lo;
    lanes.hi[lane] = shifted.hi;
  }
  *lo = _mm256_loadu_si256((const __m256i *)lanes.lo);
  *hi = _mm256_loadu_si256((const __m256i *)lanes.hi);
}

AMU_HASH_TARGET_AVX2
static inline void hash_update_data_avx2(__m256i *lo, __m256i *hi,
                                         const uint8_t *data, __m256i offsets,
                                         size_t lane_span_bytes, size_t bytes) {
  while (bytes >= HASH_BATCH_BYTES) {
    bytes -= HASH_BATCH_BYTES;
    hash_update_seven_avx2(lo, hi, hash_pack_seven_avx2(data, offsets, bytes));
  }
  while (bytes != 0) {
    --bytes;
    const __m256i symbols = _mm256_set_epi64x(
      UINT64_C(0x100) | data[3 * lane_span_bytes + bytes],
      UINT64_C(0x100) | data[2 * lane_span_bytes + bytes],
      UINT64_C(0x100) | data[lane_span_bytes + bytes], UINT64_C(0x100) | data[bytes]);
    hash_update_byte_avx2(lo, hi, symbols);
  }
}

AMU_HASH_TARGET_AVX2
static gf128_t hash_parallel_avx2(const uint8_t *reg_data, size_t reg_row_bytes,
                                size_t active_rows, size_t active_row_bytes) {
  const size_t rows_per_lane = active_rows / HASH_LANES;
  const size_t lane_span_bytes = rows_per_lane * reg_row_bytes;
  const size_t gap_bytes = reg_row_bytes - active_row_bytes;
  const bool jump_gap = gap_bytes >= HASH_AVX2_MIN_GAP_BYTES && rows_per_lane > 1;
  const gf128_t gap_factor = jump_gap ? hash_shift_factor_avx2(gap_bytes) : (gf128_t){0};
  const __m256i offsets = _mm256_set_epi64x(3 * lane_span_bytes, 2 * lane_span_bytes, lane_span_bytes, 0);
  __m256i lo = _mm256_setzero_si256(), hi = _mm256_setzero_si256();

  // Each lane hashes a contiguous group independently, removing the single state dependency.
  for (size_t row = rows_per_lane; row-- > 0;) {
    if (row + 1 != rows_per_lane) {
      if (jump_gap) hash_multiply_lanes_avx2(&lo, &hi, gap_factor);
      else hash_advance_unwritten_avx2(&lo, &hi, gap_bytes);
    }
    const uint8_t *data = reg_data + row * reg_row_bytes;
    hash_update_data_avx2(&lo, &hi, data, offsets, lane_span_bytes, active_row_bytes);
  }

  hash_lanes_t lanes;
  _mm256_storeu_si256((__m256i *)lanes.lo, lo);
  _mm256_storeu_si256((__m256i *)lanes.hi, hi);
  const gf128_t lane_factor = hash_shift_factor_avx2(lane_span_bytes);
  const size_t tail_rows = active_rows % HASH_LANES;
  gf128_t hash;
  size_t remaining_lanes = HASH_LANES;
  if (tail_rows != 0) {
    hash = hash_scalar_avx2(reg_data + HASH_LANES * lane_span_bytes, reg_row_bytes,
                            tail_rows, active_row_bytes);
  } else {
    remaining_lanes = HASH_LANES - 1;
    hash = (gf128_t){.lo = lanes.lo[HASH_LANES - 1], .hi = lanes.hi[HASH_LANES - 1]};
  }
  // Concatenation is H(high group)*x^(9*lane_span_bytes) XOR H(low group), modulo the polynomial.
  while (remaining_lanes != 0) {
    const size_t lane = --remaining_lanes;
    hash = gf128_multiply_pclmul(hash, lane_factor);
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
bool hash_amu_x86_try(const uint8_t *reg_data, size_t reg_row_bytes,
                      size_t active_rows, size_t active_row_bytes, gf128_t *result) {
  if (host_supports_fast_path && hash_can_parallelize(reg_row_bytes, active_rows)) {
    *result = hash_parallel_avx2(reg_data, reg_row_bytes, active_rows, active_row_bytes);
    return true;
  }
  return false;
}

#endif // defined(CONFIG_RV_AME) && defined(CONFIG_SHARE_REF) && defined(AMU_HASH_X86)
