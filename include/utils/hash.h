/***************************************************************************************
* Copyright (c) 2020-2026 Institute of Computing Technology, Chinese Academy of Sciences
*
* NEMU is licensed under Mulan PSL v2.
* You may obtain a copy of Mulan PSL v2 at:
*          http://license.coscl.org.cn/MulanPSL2
***************************************************************************************/

#ifndef NEMU_UTILS_HASH_H
#define NEMU_UTILS_HASH_H

#include <stddef.h>
#include <stdint.h>

// Shared hash primitives, not a standard cryptographic hash implementation.
// SplitMix64 increment, finalizer multipliers and shifts:
// https://prng.di.unimi.it/splitmix64.c
#define DIFFTEST_HASH_COUNT_STEP UINT64_C(0x9e3779b97f4a7c15)
#define DIFFTEST_HASH_MIX_MUL0 UINT64_C(0xbf58476d1ce4e5b9)
#define DIFFTEST_HASH_MIX_MUL1 UINT64_C(0x94d049bb133111eb)
#define DIFFTEST_HASH_MIX_SHIFT0 30
#define DIFFTEST_HASH_MIX_SHIFT1 27
#define DIFFTEST_HASH_MIX_SHIFT2 31

// Consecutive 32-bit Blowfish P-array words (hexadecimal digits of pi),
// concatenated into 64-bit seeds/addends for our custom composition:
// https://www.schneier.com/wp-content/uploads/2015/12/constants-2.txt
#define DIFFTEST_HASH_SEED_LO UINT64_C(0x243f6a8885a308d3) // P[0], P[1]
#define DIFFTEST_HASH_SEED_HI UINT64_C(0x13198a2e03707344) // P[2], P[3]
#define DIFFTEST_STORE_HASH_ADD_LO UINT64_C(0xa4093822299f31d0) // P[4], P[5]
#define DIFFTEST_STORE_HASH_ADD_HI UINT64_C(0x082efa98ec4e6c89) // P[6], P[7]

// XXH64 PRIME64_1 and PRIME64_2; only the constants are reused:
// https://github.com/Cyan4973/xxHash/blob/v0.8.3/xxhash.h
#define DIFFTEST_HASH_XXH_PRIME1 UINT64_C(0x9e3779b185ebca87)
#define DIFFTEST_HASH_XXH_PRIME2 UINT64_C(0xc2b2ae3d27d4eb4f)
#define DIFFTEST_HASH_WORD_BITS 64

// All callers use constant shifts in [1, 63]; shifts of 0 or 64 are invalid.
static inline uint64_t difftest_hash_rotl(uint64_t value, unsigned shift) {
  return (value << shift) | (value >> (DIFFTEST_HASH_WORD_BITS - shift));
}

// SplitMix64 finalizer: diffuse input bits with XOR shifts and multiplication.
static inline uint64_t difftest_hash_mix(uint64_t value) {
  value ^= value >> DIFFTEST_HASH_MIX_SHIFT0;
  value *= DIFFTEST_HASH_MIX_MUL0;
  value ^= value >> DIFFTEST_HASH_MIX_SHIFT1;
  value *= DIFFTEST_HASH_MIX_MUL1;
  return value ^ (value >> DIFFTEST_HASH_MIX_SHIFT2);
}

// Start both lanes from fixed seeds so independent executions agree.
static inline void difftest_hash_init(uint64_t *lo, uint64_t *hi) {
  *lo = DIFFTEST_HASH_SEED_LO;
  *hi = DIFFTEST_HASH_SEED_HI;
}

#ifdef __cplusplus
extern "C" {
#endif

// Initialize and hash a byte sequence with the existing boundary-state
// composition. Store records use their separate versioned update function.
void difftest_hash_bytes(uint64_t *lo, uint64_t *hi, const void *data, size_t size);

#ifdef __cplusplus
}
#endif

#endif // NEMU_UTILS_HASH_H
