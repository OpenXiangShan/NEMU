#ifndef NEMU_STORE_HASH_H
#define NEMU_STORE_HASH_H

#include <stdint.h>

#define DIFFTEST_STORE_HASH_VERSION 1

typedef struct {
  uint64_t h0;
  uint64_t h1;
  uint64_t count;
} DifftestStoreHashState;

static inline uint64_t difftest_store_hash_rotl(uint64_t value, unsigned int shift) {
  return (value << shift) | (value >> (64 - shift));
}

static inline uint64_t difftest_store_hash_mix(uint64_t value) {
  value ^= value >> 30;
  value *= UINT64_C(0xbf58476d1ce4e5b9);
  value ^= value >> 27;
  value *= UINT64_C(0x94d049bb133111eb);
  return value ^ (value >> 31);
}

static inline void difftest_store_hash_init(DifftestStoreHashState *state) {
  state->h0 = UINT64_C(0x243f6a8885a308d3);
  state->h1 = UINT64_C(0x13198a2e03707344);
  state->count = 0;
}

static inline void difftest_store_hash_update(DifftestStoreHashState *state, uint64_t addr, uint64_t data,
                                              uint8_t mask) {
  uint64_t value = addr ^ difftest_store_hash_rotl(data, 23) ^ ((uint64_t)mask << 56);
  value = difftest_store_hash_mix(value + state->count * UINT64_C(0x9e3779b97f4a7c15));
  state->h0 = difftest_store_hash_rotl(state->h0 ^ (value + UINT64_C(0xa4093822299f31d0)), 17) *
              UINT64_C(0x9e3779b185ebca87);
  state->h1 = difftest_store_hash_rotl(state->h1 + (value ^ UINT64_C(0x082efa98ec4e6c89)), 29) *
              UINT64_C(0xc2b2ae3d27d4eb4f);
  state->count++;
}

#endif // NEMU_STORE_HASH_H
