/***************************************************************************************
* Copyright (c) 2020-2026 Institute of Computing Technology, Chinese Academy of Sciences
*
* NEMU is licensed under Mulan PSL v2.
* You may obtain a copy of Mulan PSL v2 at:
*          http://license.coscl.org.cn/MulanPSL2
***************************************************************************************/

#include <utils.h>
#include <utils/hash.h>

// Store hash v1 is a custom ordered dual-state composition. Constant and
// primitive origins are documented in utils/hash.h. The rotations and mask
// packing are protocol choices, not parameters from the source algorithms.
// Changing the values or composition requires a store-hash version bump.
#define DIFFTEST_STORE_HASH_ROT_DATA 23
#define DIFFTEST_STORE_HASH_ROT_LO 17
#define DIFFTEST_STORE_HASH_ROT_HI 29
#define DIFFTEST_STORE_HASH_MASK_SHIFT 56

void difftest_store_hash_init(DifftestStoreHashState *state) {
  difftest_hash_init(&state->h0, &state->h1);
  state->count = 0;
}

void difftest_store_hash_update(DifftestStoreHashState *state, uint64_t addr, uint64_t data,
                                uint8_t mask) {
  uint64_t value = addr ^ difftest_hash_rotl(data, DIFFTEST_STORE_HASH_ROT_DATA) ^
                   ((uint64_t)mask << DIFFTEST_STORE_HASH_MASK_SHIFT);
  value = difftest_hash_mix(value + state->count * DIFFTEST_HASH_COUNT_STEP);
  state->h0 = difftest_hash_rotl(state->h0 ^ (value + DIFFTEST_STORE_HASH_ADD_LO),
                                 DIFFTEST_STORE_HASH_ROT_LO) * DIFFTEST_HASH_XXH_PRIME1;
  state->h1 = difftest_hash_rotl(state->h1 + (value ^ DIFFTEST_STORE_HASH_ADD_HI),
                                 DIFFTEST_STORE_HASH_ROT_HI) * DIFFTEST_HASH_XXH_PRIME2;
  state->count++;
}
