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

#if defined(CONFIG_RV_AME) && defined(CONFIG_AME_MLDST_VECTORIZE) && \
    defined(__x86_64__) && (defined(__GNUC__) || defined(__clang__))

#include "mldst_transpose_fast.h"

#define AME_TARGET_AVX2 __attribute__((target("arch=x86-64,sse2,avx2")))
#define AME_TARGET_BASELINE __attribute__((target("arch=x86-64")))
#define AME_ALWAYS_INLINE static inline __attribute__((always_inline))

#ifdef __clang__
#include <immintrin.h>
#else
#pragma GCC push_options
#pragma GCC target("arch=x86-64,sse2,avx2")
#include <immintrin.h>
#pragma GCC pop_options
#endif

typedef struct {
  __m128i low;
  __m128i high;
} unpack128_t;

typedef struct {
  __m256i low;
  __m256i high;
} unpack256_t;

typedef struct {
  size_t block_size;
  size_t element_size;
} transpose_kernel_t;

static bool host_has_avx2;

AME_TARGET_BASELINE
__attribute__((constructor))
static void detect_host_avx2(void) {
  __builtin_cpu_init();
  host_has_avx2 = __builtin_cpu_supports("avx2");
}

AME_TARGET_BASELINE
bool ame_transpose_available(void) {
  return host_has_avx2;
}

AME_TARGET_AVX2
AME_ALWAYS_INLINE unpack128_t unpack128_epi16(__m128i a, __m128i b) {
  return (unpack128_t) {
    .low = _mm_unpacklo_epi16(a, b),
    .high = _mm_unpackhi_epi16(a, b),
  };
}

AME_TARGET_AVX2
AME_ALWAYS_INLINE unpack128_t unpack128_epi32(__m128i a, __m128i b) {
  return (unpack128_t) {
    .low = _mm_unpacklo_epi32(a, b),
    .high = _mm_unpackhi_epi32(a, b),
  };
}

AME_TARGET_AVX2
AME_ALWAYS_INLINE unpack128_t unpack128_epi64(__m128i a, __m128i b) {
  return (unpack128_t) {
    .low = _mm_unpacklo_epi64(a, b),
    .high = _mm_unpackhi_epi64(a, b),
  };
}

AME_TARGET_AVX2
AME_ALWAYS_INLINE unpack256_t unpack256_epi32(__m256i a, __m256i b) {
  return (unpack256_t) {
    .low = _mm256_unpacklo_epi32(a, b),
    .high = _mm256_unpackhi_epi32(a, b),
  };
}

AME_TARGET_AVX2
AME_ALWAYS_INLINE unpack256_t unpack256_epi64(__m256i a, __m256i b) {
  return (unpack256_t) {
    .low = _mm256_unpacklo_epi64(a, b),
    .high = _mm256_unpackhi_epi64(a, b),
  };
}

AME_TARGET_AVX2
AME_ALWAYS_INLINE unpack256_t permute256x128(__m256i a, __m256i b) {
  return (unpack256_t) {
    .low = _mm256_permute2x128_si256(a, b, 0x20),
    .high = _mm256_permute2x128_si256(a, b, 0x31),
  };
}

AME_TARGET_AVX2
AME_ALWAYS_INLINE void store128_as_2x64(uint8_t *dst, size_t dst_row_stride,
                                        __m128i value) {
  _mm_storel_epi64((__m128i *)dst, value);
  _mm_storel_epi64((__m128i *)(dst + dst_row_stride),
                   _mm_srli_si128(value, 8));
}

AME_TARGET_AVX2
static void transpose_e8_8x8(const uint8_t *src, size_t src_row_stride,
                             uint8_t *dst, size_t dst_row_stride) {
  __m128i r0 = _mm_loadl_epi64((const __m128i *)(src + 0 * src_row_stride));
  __m128i r1 = _mm_loadl_epi64((const __m128i *)(src + 1 * src_row_stride));
  __m128i r2 = _mm_loadl_epi64((const __m128i *)(src + 2 * src_row_stride));
  __m128i r3 = _mm_loadl_epi64((const __m128i *)(src + 3 * src_row_stride));
  __m128i r4 = _mm_loadl_epi64((const __m128i *)(src + 4 * src_row_stride));
  __m128i r5 = _mm_loadl_epi64((const __m128i *)(src + 5 * src_row_stride));
  __m128i r6 = _mm_loadl_epi64((const __m128i *)(src + 6 * src_row_stride));
  __m128i r7 = _mm_loadl_epi64((const __m128i *)(src + 7 * src_row_stride));

  __m128i rows01 = _mm_unpacklo_epi8(r0, r1);
  __m128i rows23 = _mm_unpacklo_epi8(r2, r3);
  __m128i rows45 = _mm_unpacklo_epi8(r4, r5);
  __m128i rows67 = _mm_unpacklo_epi8(r6, r7);
  unpack128_t groups0123 = unpack128_epi16(rows01, rows23);
  unpack128_t groups4567 = unpack128_epi16(rows45, rows67);
  unpack128_t columns03 = unpack128_epi32(groups0123.low, groups4567.low);
  unpack128_t columns47 = unpack128_epi32(groups0123.high, groups4567.high);

  store128_as_2x64(dst + 0 * dst_row_stride, dst_row_stride, columns03.low);
  store128_as_2x64(dst + 2 * dst_row_stride, dst_row_stride, columns03.high);
  store128_as_2x64(dst + 4 * dst_row_stride, dst_row_stride, columns47.low);
  store128_as_2x64(dst + 6 * dst_row_stride, dst_row_stride, columns47.high);
}

AME_TARGET_AVX2
static void transpose_e16_8x8(const uint8_t *src, size_t src_row_stride,
                              uint8_t *dst, size_t dst_row_stride) {
  __m128i r0 = _mm_loadu_si128((const __m128i *)(src + 0 * src_row_stride));
  __m128i r1 = _mm_loadu_si128((const __m128i *)(src + 1 * src_row_stride));
  __m128i r2 = _mm_loadu_si128((const __m128i *)(src + 2 * src_row_stride));
  __m128i r3 = _mm_loadu_si128((const __m128i *)(src + 3 * src_row_stride));
  __m128i r4 = _mm_loadu_si128((const __m128i *)(src + 4 * src_row_stride));
  __m128i r5 = _mm_loadu_si128((const __m128i *)(src + 5 * src_row_stride));
  __m128i r6 = _mm_loadu_si128((const __m128i *)(src + 6 * src_row_stride));
  __m128i r7 = _mm_loadu_si128((const __m128i *)(src + 7 * src_row_stride));

  unpack128_t rows01 = unpack128_epi16(r0, r1);
  unpack128_t rows23 = unpack128_epi16(r2, r3);
  unpack128_t rows45 = unpack128_epi16(r4, r5);
  unpack128_t rows67 = unpack128_epi16(r6, r7);
  unpack128_t groups0123_low = unpack128_epi32(rows01.low, rows23.low);
  unpack128_t groups0123_high = unpack128_epi32(rows01.high, rows23.high);
  unpack128_t groups4567_low = unpack128_epi32(rows45.low, rows67.low);
  unpack128_t groups4567_high = unpack128_epi32(rows45.high, rows67.high);
  unpack128_t columns01 = unpack128_epi64(groups0123_low.low, groups4567_low.low);
  unpack128_t columns23 = unpack128_epi64(groups0123_low.high, groups4567_low.high);
  unpack128_t columns45 = unpack128_epi64(groups0123_high.low, groups4567_high.low);
  unpack128_t columns67 = unpack128_epi64(groups0123_high.high, groups4567_high.high);

  _mm_storeu_si128((__m128i *)(dst + 0 * dst_row_stride), columns01.low);
  _mm_storeu_si128((__m128i *)(dst + 1 * dst_row_stride), columns01.high);
  _mm_storeu_si128((__m128i *)(dst + 2 * dst_row_stride), columns23.low);
  _mm_storeu_si128((__m128i *)(dst + 3 * dst_row_stride), columns23.high);
  _mm_storeu_si128((__m128i *)(dst + 4 * dst_row_stride), columns45.low);
  _mm_storeu_si128((__m128i *)(dst + 5 * dst_row_stride), columns45.high);
  _mm_storeu_si128((__m128i *)(dst + 6 * dst_row_stride), columns67.low);
  _mm_storeu_si128((__m128i *)(dst + 7 * dst_row_stride), columns67.high);
}

AME_TARGET_AVX2
static void transpose_e32_8x8(const uint8_t *src, size_t src_row_stride,
                              uint8_t *dst, size_t dst_row_stride) {
  __m256i r0 = _mm256_loadu_si256((const __m256i *)(src + 0 * src_row_stride));
  __m256i r1 = _mm256_loadu_si256((const __m256i *)(src + 1 * src_row_stride));
  __m256i r2 = _mm256_loadu_si256((const __m256i *)(src + 2 * src_row_stride));
  __m256i r3 = _mm256_loadu_si256((const __m256i *)(src + 3 * src_row_stride));
  __m256i r4 = _mm256_loadu_si256((const __m256i *)(src + 4 * src_row_stride));
  __m256i r5 = _mm256_loadu_si256((const __m256i *)(src + 5 * src_row_stride));
  __m256i r6 = _mm256_loadu_si256((const __m256i *)(src + 6 * src_row_stride));
  __m256i r7 = _mm256_loadu_si256((const __m256i *)(src + 7 * src_row_stride));

  unpack256_t rows01 = unpack256_epi32(r0, r1);
  unpack256_t rows23 = unpack256_epi32(r2, r3);
  unpack256_t rows45 = unpack256_epi32(r4, r5);
  unpack256_t rows67 = unpack256_epi32(r6, r7);
  unpack256_t groups0123_low = unpack256_epi64(rows01.low, rows23.low);
  unpack256_t groups0123_high = unpack256_epi64(rows01.high, rows23.high);
  unpack256_t groups4567_low = unpack256_epi64(rows45.low, rows67.low);
  unpack256_t groups4567_high = unpack256_epi64(rows45.high, rows67.high);
  unpack256_t columns04 = permute256x128(groups0123_low.low, groups4567_low.low);
  unpack256_t columns15 = permute256x128(groups0123_low.high, groups4567_low.high);
  unpack256_t columns26 = permute256x128(groups0123_high.low, groups4567_high.low);
  unpack256_t columns37 = permute256x128(groups0123_high.high, groups4567_high.high);

  _mm256_storeu_si256((__m256i *)(dst + 0 * dst_row_stride), columns04.low);
  _mm256_storeu_si256((__m256i *)(dst + 1 * dst_row_stride), columns15.low);
  _mm256_storeu_si256((__m256i *)(dst + 2 * dst_row_stride), columns26.low);
  _mm256_storeu_si256((__m256i *)(dst + 3 * dst_row_stride), columns37.low);
  _mm256_storeu_si256((__m256i *)(dst + 4 * dst_row_stride), columns04.high);
  _mm256_storeu_si256((__m256i *)(dst + 5 * dst_row_stride), columns15.high);
  _mm256_storeu_si256((__m256i *)(dst + 6 * dst_row_stride), columns26.high);
  _mm256_storeu_si256((__m256i *)(dst + 7 * dst_row_stride), columns37.high);
}

AME_TARGET_AVX2
static void transpose_e64_4x4(const uint8_t *src, size_t src_row_stride,
                              uint8_t *dst, size_t dst_row_stride) {
  __m256i r0 = _mm256_loadu_si256((const __m256i *)(src + 0 * src_row_stride));
  __m256i r1 = _mm256_loadu_si256((const __m256i *)(src + 1 * src_row_stride));
  __m256i r2 = _mm256_loadu_si256((const __m256i *)(src + 2 * src_row_stride));
  __m256i r3 = _mm256_loadu_si256((const __m256i *)(src + 3 * src_row_stride));

  unpack256_t rows01 = unpack256_epi64(r0, r1);
  unpack256_t rows23 = unpack256_epi64(r2, r3);
  unpack256_t columns02 = permute256x128(rows01.low, rows23.low);
  unpack256_t columns13 = permute256x128(rows01.high, rows23.high);

  _mm256_storeu_si256((__m256i *)(dst + 0 * dst_row_stride), columns02.low);
  _mm256_storeu_si256((__m256i *)(dst + 1 * dst_row_stride), columns13.low);
  _mm256_storeu_si256((__m256i *)(dst + 2 * dst_row_stride), columns02.high);
  _mm256_storeu_si256((__m256i *)(dst + 3 * dst_row_stride), columns13.high);
}

AME_TARGET_AVX2
static bool get_transpose_kernel(int msew, transpose_kernel_t *kernel) {
  static const transpose_kernel_t kernels[] = {
    { .block_size = 8, .element_size = 1 },
    { .block_size = 8, .element_size = 2 },
    { .block_size = 8, .element_size = 4 },
    { .block_size = 4, .element_size = 8 },
  };

  if ((unsigned)msew >= sizeof(kernels) / sizeof(kernels[0])) {
    return false;
  }
  *kernel = kernels[msew];
  return true;
}

AME_TARGET_AVX2
AME_ALWAYS_INLINE void transpose_blocks(
  const uint8_t *src, size_t src_row_stride,
  uint8_t *dst, size_t dst_row_stride,
  size_t full_rows, size_t full_columns,
  transpose_kernel_t kernel, int msew
) {
  for (size_t block_row = 0; block_row < full_rows;
       block_row += kernel.block_size) {
    for (size_t block_column = 0; block_column < full_columns;
         block_column += kernel.block_size) {
      const uint8_t *block_src =
        src + block_row * src_row_stride + block_column * kernel.element_size;
      uint8_t *block_dst =
        dst + block_column * dst_row_stride + block_row * kernel.element_size;
      switch (msew) {
        case 0:
          transpose_e8_8x8(block_src, src_row_stride,
                           block_dst, dst_row_stride);
          break;
        case 1:
          transpose_e16_8x8(block_src, src_row_stride,
                            block_dst, dst_row_stride);
          break;
        case 2:
          transpose_e32_8x8(block_src, src_row_stride,
                            block_dst, dst_row_stride);
          break;
        case 3:
          transpose_e64_4x4(block_src, src_row_stride,
                            block_dst, dst_row_stride);
          break;
        default:
          __builtin_unreachable();
      }
    }
  }
}

AME_TARGET_AVX2
static void transpose_scalar_edges(
  const uint8_t *src, size_t src_row_stride,
  uint8_t *dst, size_t dst_row_stride,
  size_t rows, size_t columns,
  size_t full_rows, size_t full_columns,
  size_t element_size
) {
  for (size_t r = 0; r < full_rows; r++) {
    for (size_t c = full_columns; c < columns; c++) {
      __builtin_memcpy(dst + c * dst_row_stride + r * element_size,
                       src + r * src_row_stride + c * element_size,
                       element_size);
    }
  }
  for (size_t r = full_rows; r < rows; r++) {
    for (size_t c = 0; c < columns; c++) {
      __builtin_memcpy(dst + c * dst_row_stride + r * element_size,
                       src + r * src_row_stride + c * element_size,
                       element_size);
    }
  }
}

AME_TARGET_AVX2
bool ame_transpose_matrix(
  const uint8_t *src, size_t src_row_stride,
  uint8_t *dst, size_t dst_row_stride,
  size_t rows, size_t columns, int msew
) {
  transpose_kernel_t kernel;
  if (!get_transpose_kernel(msew, &kernel)) {
    return false;
  }

  size_t full_rows = rows / kernel.block_size * kernel.block_size;
  size_t full_columns = columns / kernel.block_size * kernel.block_size;
  if (full_rows == 0 || full_columns == 0) {
    return false;
  }

  switch (msew) {
    case 0:
      transpose_blocks(src, src_row_stride, dst, dst_row_stride,
                       full_rows, full_columns, kernel, 0);
      break;
    case 1:
      transpose_blocks(src, src_row_stride, dst, dst_row_stride,
                       full_rows, full_columns, kernel, 1);
      break;
    case 2:
      transpose_blocks(src, src_row_stride, dst, dst_row_stride,
                       full_rows, full_columns, kernel, 2);
      break;
    case 3:
      transpose_blocks(src, src_row_stride, dst, dst_row_stride,
                       full_rows, full_columns, kernel, 3);
      break;
    default:
      return false;
  }

  transpose_scalar_edges(src, src_row_stride, dst, dst_row_stride,
                         rows, columns, full_rows, full_columns,
                         kernel.element_size);
  return true;
}

#undef AME_ALWAYS_INLINE
#undef AME_TARGET_BASELINE
#undef AME_TARGET_AVX2

#endif
