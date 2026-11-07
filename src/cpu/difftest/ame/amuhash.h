#ifndef CPU_DIFFTEST_AME_AMUHASH_H
#define CPU_DIFFTEST_AME_AMUHASH_H

#include <generated/autoconf.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#if defined(__x86_64__) && (defined(__GNUC__) || defined(__clang__))
#define AMU_HASH_X86
// Keep these feature bundles aligned with detect_hash_host_features().
#define AMU_HASH_TARGET_BASELINE __attribute__((target("arch=x86-64")))
#define AMU_HASH_TARGET_AVX2 __attribute__((target("arch=x86-64,avx2,sse4.2,pclmul,bmi2")))
#define AMU_HASH_TARGET_AVX512 __attribute__((target("arch=x86-64,avx2,sse4.2,pclmul,bmi2,avx512f,avx512dq,vpclmulqdq")))
#else
#define AMU_HASH_TARGET_BASELINE
#endif

#define HASH_SYMBOL_BITS 9
#define HASH_BATCH_BYTES 7
#define HASH_LANES 4

#define HASH_PORTABLE_MIN_GAP_BYTES 128
#define HASH_PARALLEL_MIN_REGION_BYTES 1024

// Kernels return a representative of sum(symbol[i] * x^(9*i)) modulo
// x^32+0x1edc6f41. Initialization, x^32 and reflected finalization are shared.

#if defined(CONFIG_RV_AME) && defined(CONFIG_SHARE_REF)
uint64_t hash_amu_portable(const uint8_t *reg_data, size_t reg_row_bytes,
                          size_t active_rows, size_t active_row_bytes);
#ifdef AMU_HASH_X86
bool hash_amu_x86_try(const uint8_t *reg_data, size_t reg_row_bytes,
                      size_t active_rows, size_t active_row_bytes, uint64_t *result);
#endif
#endif

#endif // CPU_DIFFTEST_AME_AMUHASH_H
