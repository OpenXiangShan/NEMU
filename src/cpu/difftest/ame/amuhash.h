#ifndef CPU_DIFFTEST_AME_AMUHASH_H
#define CPU_DIFFTEST_AME_AMUHASH_H

#include <generated/autoconf.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#if defined(__x86_64__) && (defined(__GNUC__) || defined(__clang__))
#define AMU_HASH_X86
#define AMU_HASH_TARGET_BASELINE __attribute__((target("arch=x86-64")))
#define AMU_HASH_TARGET_AVX2 __attribute__((target("arch=x86-64,sse2,avx2,pclmul")))
#else
#define AMU_HASH_TARGET_BASELINE
#endif

#define HASH_SYMBOL_BITS 9
#define HASH_BATCH_BYTES 7
#define HASH_LANES 4

// Select kernels by region size and unwritten bytes per row.
#define HASH_AVX2_MIN_GAP_BYTES 64
#define HASH_PORTABLE_MIN_GAP_BYTES 256
#define HASH_PARALLEL_MIN_REGION_BYTES 1024
#define HASH_SMALL_SPARSE_ROW_LIMIT 32
#define HASH_SMALL_SPARSE_MAX_ACTIVE_ROW_BYTES 16

// Remainder modulo x^128+x^7+x^2+x+1, separate from the covered-byte count.
typedef struct {
  uint64_t lo;
  uint64_t hi;
} gf128_t;

typedef struct {
  gf128_t multiples[16];
} gf128_multiplier_t;

typedef struct {
  uint64_t lo[HASH_LANES];
  uint64_t hi[HASH_LANES];
} hash_lanes_t;

#if defined(CONFIG_RV_AME) && defined(CONFIG_SHARE_REF)
gf128_t hash_amu_portable(const uint8_t *reg_data, size_t reg_row_bytes,
                          size_t active_rows, size_t active_row_bytes);
#ifdef AMU_HASH_X86
bool hash_amu_x86_try(const uint8_t *reg_data, size_t reg_row_bytes,
                      size_t active_rows, size_t active_row_bytes, gf128_t *result);
#endif
#endif

#endif // CPU_DIFFTEST_AME_AMUHASH_H
