/***************************************************************************************
* Copyright (c) 2020-2026 Institute of Computing Technology, Chinese Academy of Sciences
*
* NEMU is licensed under Mulan PSL v2.
* You may obtain a copy of Mulan PSL v2 at:
*          http://license.coscl.org.cn/MulanPSL2
***************************************************************************************/

#include <utils/hash.h>

// FNV-64 prime (2^40 + 2^8 + 0xb3), reused as a multiplier:
// https://www.ietf.org/archive/id/draft-eastlake-fnv-25.html#section-2.1
#define DIFFTEST_STATE_HASH_MUL_LO UINT64_C(0x100000001b3)

// SHA-512 initial words H[0..3]:
// https://www.rfc-editor.org/rfc/rfc6234#section-6.3
// The low-lane salt is the custom odd variant H[0] | 1, NOT SHA-512's H[0].
#define DIFFTEST_STATE_HASH_SALT_LO (UINT64_C(0x6a09e667f3bcc908) | UINT64_C(1))
#define DIFFTEST_STATE_HASH_SALT_HI UINT64_C(0xbb67ae8584caa73b)
#define DIFFTEST_STATE_HASH_ADD_LO UINT64_C(0x3c6ef372fe94f82b)
#define DIFFTEST_STATE_HASH_ADD_HI UINT64_C(0xa54ff53a5f1d36f1)

// Custom boundary-state rotations/composition, preserved for compatibility.
#define DIFFTEST_STATE_HASH_ROT_LO 29
#define DIFFTEST_STATE_HASH_ROT_HI 31

void difftest_hash_bytes(uint64_t *lo, uint64_t *hi, const void *data, size_t size) {
  const uint8_t *bytes = (const uint8_t *)data;
  difftest_hash_init(lo, hi);
  for (size_t index = 0; index < size; ++index) {
    const uint64_t value = bytes[index] ^ (DIFFTEST_HASH_COUNT_STEP * (index + 1));
    *lo = difftest_hash_rotl(*lo ^ difftest_hash_mix(value + DIFFTEST_STATE_HASH_SALT_LO),
                             DIFFTEST_STATE_HASH_ROT_LO);
    *lo = *lo * DIFFTEST_STATE_HASH_MUL_LO + DIFFTEST_STATE_HASH_ADD_LO;
    *hi = difftest_hash_rotl(*hi + difftest_hash_mix(value ^ DIFFTEST_STATE_HASH_SALT_HI),
                             DIFFTEST_STATE_HASH_ROT_HI);
    *hi = *hi * DIFFTEST_HASH_XXH_PRIME1 + DIFFTEST_STATE_HASH_ADD_HI;
  }
}
