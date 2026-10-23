/***************************************************************************************
* Copyright (c) 2020-2026 Institute of Computing Technology, Chinese Academy of Sciences
*
* NEMU is licensed under Mulan PSL v2.
* You may obtain a copy of Mulan PSL v2 at:
*          http://license.coscl.org.cn/MulanPSL2
***************************************************************************************/

#include <memory/store_log_hash.h>

#ifdef CONFIG_STORE_LOG_HASH
#define XXH_STATIC_LINKING_ONLY
#include <xxhash.h>

// Fixed-width records avoid struct padding; batch updates amortize XXH3 calls.
#define STORE_HASH_RECORD_BYTES 17
#define STORE_HASH_BATCH_RECORDS 256

typedef struct {
  XXH3_state_t state;
  uint64_t count;
  uint8_t pending[STORE_HASH_BATCH_RECORDS * STORE_HASH_RECORD_BYTES];
} StoreLogHash;

static StoreLogHash boundary_hash;
static bool hash_enabled = true;
#ifdef CONFIG_STORE_LOG
static StoreLogHash rollback_hash;
#endif

void store_log_hash_reset(void) {
  XXH3_128bits_reset(&boundary_hash.state);
  boundary_hash.count = 0;
  IFDEF(CONFIG_STORE_LOG, rollback_hash = boundary_hash);
}

void store_log_hash_set_enabled(bool enabled) {
  hash_enabled = enabled;
}

bool store_log_hash_enabled(void) {
  return hash_enabled;
}

void store_log_hash_update(uint64_t addr, uint64_t data, uint8_t mask) {
  if (!hash_enabled) return;
  uint8_t *record = boundary_hash.pending +
                    (boundary_hash.count % STORE_HASH_BATCH_RECORDS) * STORE_HASH_RECORD_BYTES;
  // addr || data || mask, with both 64-bit fields serialized little-endian.
  for (unsigned i = 0; i < 8; ++i) {
    record[i] = addr >> (8 * i);
    record[8 + i] = data >> (8 * i);
  }
  record[16] = mask;
  boundary_hash.count++;
  if (boundary_hash.count % STORE_HASH_BATCH_RECORDS == 0)
    XXH3_128bits_update(&boundary_hash.state, boundary_hash.pending, sizeof(boundary_hash.pending));
}

void store_log_hash(uint64_t *lo, uint64_t *hi, uint64_t *count) {
  // Query a copy so the pending tail remains valid for later writes or forks.
  XXH3_state_t state;
  XXH3_copyState(&state, &boundary_hash.state);
  XXH3_128bits_update(&state, boundary_hash.pending,
                      (boundary_hash.count % STORE_HASH_BATCH_RECORDS) * STORE_HASH_RECORD_BYTES);
  XXH128_hash_t hash = XXH3_128bits_digest(&state);
  *lo = hash.low64;
  *hi = hash.high64;
  *count = boundary_hash.count;
}

#ifdef CONFIG_STORE_LOG
void store_log_hash_checkpoint(void) {
  rollback_hash = boundary_hash;
}

void store_log_hash_restore(void) {
  boundary_hash = rollback_hash;
}
#endif
#endif
