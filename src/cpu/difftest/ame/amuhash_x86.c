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

static enum {
  HASH_HOST_BASELINE,
  HASH_HOST_AVX2,
  HASH_HOST_AVX512,
} host_hash_tier;

AMU_HASH_TARGET_BASELINE
__attribute__((constructor))
static void detect_hash_host_features(void) {
  __builtin_cpu_init();
  if (!(__builtin_cpu_supports("avx2") && __builtin_cpu_supports("sse4.2") &&
        __builtin_cpu_supports("pclmul") && __builtin_cpu_supports("bmi2"))) return;
  host_hash_tier = HASH_HOST_AVX2;
  if (__builtin_cpu_supports("avx512f") && __builtin_cpu_supports("avx512dq") &&
      __builtin_cpu_supports("vpclmulqdq")) host_hash_tier = HASH_HOST_AVX512;
}

AMU_HASH_TARGET_AVX2
static inline uint32_t hash_reduce64_pclmul(uint64_t value) {
  // Reciprocal polynomial floor(x^64/P) = 0x11f91caf6. The high word
  // determines the quotient; only the low word of quotient*P is needed.
  const __m128i quotient = _mm_srli_epi64(_mm_clmulepi64_si128(
    _mm_cvtsi64_si128(value >> 32), _mm_cvtsi64_si128(UINT64_C(0x11f91caf6)), 0), 32);
  const __m128i product = _mm_clmulepi64_si128(quotient,
    _mm_cvtsi64_si128(UINT64_C(0x11edc6f41)), 0);
  return value ^ (uint64_t)_mm_cvtsi128_si64(product);
}

AMU_HASH_TARGET_AVX2
static inline uint32_t hash_reduce128_pclmul(uint64_t low, uint64_t high) {
  const __m128i factor = _mm_cvtsi64_si128(UINT64_C(0x3aab4576)); // x^64 mod P
  const __m128i first = _mm_clmulepi64_si128(_mm_cvtsi64_si128(high), factor, 0);
  const __m128i second = _mm_clmulepi64_si128(_mm_srli_si128(first, 8), factor, 0);
  return hash_reduce64_pclmul(low ^ (uint64_t)_mm_cvtsi128_si64(first) ^
                             (uint64_t)_mm_cvtsi128_si64(second));
}

AMU_HASH_TARGET_AVX2
static inline uint32_t gf32_multiply_pclmul(uint32_t a, uint32_t b) {
  const __m128i product = _mm_clmulepi64_si128(_mm_cvtsi64_si128(a), _mm_cvtsi64_si128(b), 0);
  return hash_reduce64_pclmul((uint64_t)_mm_cvtsi128_si64(product));
}

AMU_HASH_TARGET_AVX2
static uint64_t hash_shift_factor_avx2(size_t bytes) {
  uint64_t factor = 1;
  for (unsigned bit = 0; bytes; ++bit, bytes >>= 1) {
    if (bytes & 1) factor = factor == 1 ? hash_shift_powers[bit] :
                           gf32_multiply_pclmul(factor, hash_shift_powers[bit]);
  }
  return factor;
}

AMU_HASH_TARGET_AVX2
static inline uint64_t hash_pack_seven_bmi2(const uint8_t *data, size_t byte) {
  const uint64_t packed = byte ? hash_load_le64(data + byte - 1) >> 8 : hash_load_le64(data);
  return _pdep_u64(packed, UINT64_C(0x3fdfeff7fbfdfeff)) | UINT64_C(0x4020100804020100);
}

AMU_HASH_TARGET_AVX2
static inline __m256i hash_pack_seven_fold(const uint8_t *data, size_t span, size_t byte) {
  return _mm256_set_epi64x(hash_pack_seven_bmi2(data + 3 * span, byte),
    hash_pack_seven_bmi2(data + 2 * span, byte), hash_pack_seven_bmi2(data + span, byte),
    hash_pack_seven_bmi2(data, byte));
}

// Keep unreduced 128-bit lane states between blocks. Four VPCLMUL instructions
// advance all four lanes by fourteen symbols; only final states need reduction.
AMU_HASH_TARGET_AVX512
static inline void hash_fold_lanes(__m256i *lo, __m256i *hi, uint64_t low_factor,
                                   uint64_t high_factor, __m256i input_lo, __m256i input_hi) {
  const __m256i a = _mm256_set1_epi64x(low_factor), b = _mm256_set1_epi64x(high_factor);
  const __m256i even = _mm256_xor_si256(_mm256_clmulepi64_epi128(*lo, a, 0x00),
                                       _mm256_clmulepi64_epi128(*hi, b, 0x00));
  const __m256i odd = _mm256_xor_si256(_mm256_clmulepi64_epi128(*lo, a, 0x11),
                                      _mm256_clmulepi64_epi128(*hi, b, 0x11));
  *lo = _mm256_xor_si256(_mm256_unpacklo_epi64(even, odd), input_lo);
  *hi = _mm256_xor_si256(_mm256_unpackhi_epi64(even, odd), input_hi);
}

AMU_HASH_TARGET_AVX512
static uint64_t hash_parallel_fold(const uint8_t *data, size_t stride, size_t rows, size_t bytes) {
  size_t rows_per_lane, span, tail;
  if (bytes == stride) {
    // A full tile is contiguous; remove per-row tails and gaps entirely.
    span = rows * stride / HASH_LANES;
    tail = 0; // Dispatch only flattens a stride divisible by HASH_LANES.
    rows_per_lane = 1;
    bytes = stride = span;
  } else {
    rows_per_lane = rows / HASH_LANES;
    span = rows_per_lane * stride;
    tail = rows % HASH_LANES;
  }
  const size_t gap = stride - bytes;
  const uint64_t gap_factor = gap ? hash_shift_factor_avx2(gap) : 0;
  __m256i lo = _mm256_setzero_si256(), hi = _mm256_setzero_si256();
  for (size_t row = rows_per_lane; row-- > 0;) {
    if (gap && row + 1 != rows_per_lane)
      hash_fold_lanes(&lo, &hi, gap_factor, gf32_fold(gap_factor),
                       _mm256_setzero_si256(), _mm256_setzero_si256());
    const uint8_t *p = data + row * stride;
    size_t left = bytes;
    while (left >= 14) {
      left -= 14;
      const __m256i lower = hash_pack_seven_fold(p, span, left);
      const __m256i upper = hash_pack_seven_fold(p, span, left + 7);
      hash_fold_lanes(&lo, &hi, UINT64_C(0x0615c746), UINT64_C(0x5ef7e176),
        _mm256_xor_si256(lower, _mm256_slli_epi64(upper, 63)), _mm256_srli_epi64(upper, 1));
    }
    if (left >= 7) {
      left -= 7;
      hash_fold_lanes(&lo, &hi, UINT64_C(0x1d55a2bb), UINT64_C(0x0c2b8e8c),
                       hash_pack_seven_fold(p, span, left), _mm256_setzero_si256());
    }
    if (left) {
      const __m256i input = _mm256_and_si256(hash_pack_seven_fold(p, span, 0),
        _mm256_set1_epi64x((UINT64_C(1) << (9 * left)) - 1));
      const uint64_t factor = UINT64_C(1) << (9 * left);
      hash_fold_lanes(&lo, &hi, factor, gf32_fold(factor),
                       input, _mm256_setzero_si256());
    }
  }
  uint64_t low[HASH_LANES], high[HASH_LANES];
  _mm256_storeu_si256((__m256i *)low, lo);
  _mm256_storeu_si256((__m256i *)high, hi);
  const uint64_t factor = hash_shift_factor_avx2(span);
  uint64_t result;
  unsigned remaining;
  if (tail) {
    result = gf32_reduce(hash_amu_portable(data + HASH_LANES * span, stride, tail, bytes));
    remaining = HASH_LANES;
  } else {
    result = hash_reduce128_pclmul(low[HASH_LANES - 1], high[HASH_LANES - 1]);
    remaining = HASH_LANES - 1;
  }
  while (remaining) {
    --remaining;
    result = gf32_multiply_pclmul(result, factor) ^ hash_reduce128_pclmul(low[remaining], high[remaining]);
  }
  return result;
}

AMU_HASH_TARGET_AVX512
static inline __m512i hash_wide_input(__m256i low, __m256i high) {
  return _mm512_inserti64x4(_mm512_castsi256_si512(_mm256_unpacklo_epi64(low, high)),
                            _mm256_unpackhi_epi64(low, high), 1);
}

AMU_HASH_TARGET_AVX512
static inline __m512i hash_wide_fold(__m512i state, uint64_t low_factor, uint64_t high_factor,
                                    __m512i input) {
  const __m512i factors = _mm512_set_epi64(high_factor, low_factor, high_factor, low_factor,
                                          high_factor, low_factor, high_factor, low_factor);
  return _mm512_xor_si512(input, _mm512_xor_si512(_mm512_clmulepi64_epi128(state, factors, 0x00),
                                                _mm512_clmulepi64_epi128(state, factors, 0x11)));
}

// Full matrices only: four 128-bit states fit in one ZMM register. Two wide
// carry-less products advance all lanes, avoiding the YMM state's transposition.
AMU_HASH_TARGET_AVX512
static uint64_t hash_full_wide(const uint8_t *data, size_t total) {
  const size_t span = total / HASH_LANES;
  __m512i state = _mm512_setzero_si512();
  size_t left = span;
  while (left >= 14) {
    left -= 14;
    const __m256i lower = hash_pack_seven_fold(data, span, left);
    const __m256i upper = hash_pack_seven_fold(data, span, left + 7);
    const __m512i input = hash_wide_input(_mm256_xor_si256(lower, _mm256_slli_epi64(upper, 63)),
                                         _mm256_srli_epi64(upper, 1));
    state = hash_wide_fold(state, UINT64_C(0x0615c746), UINT64_C(0x5ef7e176), input);
  }
  if (left >= 7) {
    left -= 7;
    state = hash_wide_fold(state, UINT64_C(0x1d55a2bb), UINT64_C(0x0c2b8e8c),
      hash_wide_input(hash_pack_seven_fold(data, span, left), _mm256_setzero_si256()));
  }
  if (left) {
    const __m256i input = _mm256_and_si256(hash_pack_seven_fold(data, span, 0),
      _mm256_set1_epi64x((UINT64_C(1) << (9 * left)) - 1));
    const uint64_t factor = UINT64_C(1) << (9 * left);
    state = hash_wide_fold(state, factor, gf32_fold(factor),
      hash_wide_input(input, _mm256_setzero_si256()));
  }
  uint64_t lanes[2 * HASH_LANES];
  _mm512_storeu_si512((void *)lanes, state);
  // Unpack low/high interleaves lane order 0,2,1,3.
  const unsigned offsets[HASH_LANES] = {0,4,2,6};
  const uint32_t factor = hash_shift_factor_avx2(span);
  uint32_t result = hash_reduce128_pclmul(lanes[6], lanes[7]);
  for (unsigned lane = HASH_LANES - 1; lane-- > 0;) {
    const unsigned i = offsets[lane];
    result = gf32_multiply_pclmul(result, factor) ^ hash_reduce128_pclmul(lanes[i], lanes[i + 1]);
  }
  return result;
}

AMU_HASH_TARGET_AVX2
static inline __m128i hash_reverse_pair(uint64_t a, uint64_t b) {
  const __m128i lookup = _mm_setr_epi8(0,8,4,12,2,10,6,14,1,9,5,13,3,11,7,15);
  const __m128i mask = _mm_set1_epi8(15);
  __m128i value = _mm_set_epi64x(b, a);
  value = _mm_shuffle_epi8(value, _mm_setr_epi8(7,6,5,4,3,2,1,0,15,14,13,12,11,10,9,8));
  return _mm_or_si128(_mm_slli_epi16(_mm_shuffle_epi8(lookup, _mm_and_si128(value, mask)), 4),
    _mm_shuffle_epi8(lookup, _mm_and_si128(_mm_srli_epi16(value, 4), mask)));
}

AMU_HASH_TARGET_AVX2
static inline uint64_t hash_crc_eight(uint64_t crc, uint64_t word) {
  const uint64_t low = _pdep_u64(word, UINT64_C(0x7fbfdfeff7fbfdfe)) |
                        UINT64_C(0x8040201008040201);
  return _mm_crc32_u8(_mm_crc32_u64(crc, low), word >> 56);
}

AMU_HASH_TARGET_AVX2
static inline uint32_t hash_multiply32(uint32_t a, uint32_t b) {
  const __m128i product = _mm_clmulepi64_si128(_mm_cvtsi64_si128(a), _mm_cvtsi64_si128(b), 0);
  return gf32_reduce((uint64_t)_mm_cvtsi128_si64(product));
}

// Each eight matrix bytes become nine canonical reflected bytes. Four independent
// CRC32 instruction chains hide dependency latency; no temporary packed matrix.
AMU_HASH_TARGET_AVX2
static uint64_t hash_parallel_avx2(const uint8_t *data, size_t total) {
  const size_t span = total / HASH_LANES;
  uint64_t c0 = 0, c1 = 0, c2 = 0, c3 = 0;
  for (size_t left = span; left;) {
    left -= 8;
    const __m128i a = hash_reverse_pair(hash_load_le64(data + left), hash_load_le64(data + span + left));
    const __m128i b = hash_reverse_pair(hash_load_le64(data + 2 * span + left),
                                       hash_load_le64(data + 3 * span + left));
    c0 = hash_crc_eight(c0, _mm_cvtsi128_si64(a));
    c1 = hash_crc_eight(c1, _mm_extract_epi64(a, 1));
    c2 = hash_crc_eight(c2, _mm_cvtsi128_si64(b));
    c3 = hash_crc_eight(c3, _mm_extract_epi64(b, 1));
  }
  const uint64_t crc[HASH_LANES] = {c0, c1, c2, c3};
  // CRC32 returns reverse(rem(M*x^32,P)). Undo x^32 to use the shared
  // polynomial-combination/finalization contract; 0x7e6b086b = x^-32 mod P.
  const uint64_t factor = hash_shift_factor(span);
  uint64_t result = 0;
  for (unsigned lane = HASH_LANES; lane-- > 0;) {
    const uint32_t value = hash_multiply32(hash_reverse(crc[lane]) >> 32, UINT32_C(0x7e6b086b));
    result = hash_multiply32(result, factor) ^ value;
  }
  return result;
}

AMU_HASH_TARGET_BASELINE
bool hash_amu_x86_try(const uint8_t *data, size_t stride, size_t rows, size_t bytes, uint64_t *result) {
  if (host_hash_tier == HASH_HOST_AVX512 && bytes == stride && rows * bytes >= 128 && !(bytes % 4)) {
    *result = hash_full_wide(data, rows * bytes);
    return true;
  }
  if (host_hash_tier == HASH_HOST_AVX512 && ((bytes == stride && rows * bytes >= 128 && !(bytes % 4)) ||
      (bytes >= 8 && rows >= HASH_LANES && rows * stride >= HASH_PARALLEL_MIN_REGION_BYTES))) {
    *result = hash_parallel_fold(data, stride, rows, bytes);
    return true;
  }
  if (host_hash_tier >= HASH_HOST_AVX2 && bytes == stride && rows * bytes >= 128 && !(rows * bytes % 32)) {
    *result = hash_parallel_avx2(data, rows * bytes);
    return true;
  }
  return false;
}

#endif
