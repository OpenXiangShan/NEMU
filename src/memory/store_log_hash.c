/***************************************************************************************
* Copyright (c) 2020-2026 Institute of Computing Technology, Chinese Academy of Sciences
*
* NEMU is licensed under Mulan PSL v2.
* You may obtain a copy of Mulan PSL v2 at:
*          http://license.coscl.org.cn/MulanPSL2
***************************************************************************************/

#include <memory/store_log_hash.h>
#include <utils/hash.h>

#ifdef CONFIG_STORE_LOG_HASH
// Keep the cheap checkpoint digest independent of the DUT CRC queue protocol.
// Custom ordered composition; primitive origins are documented in utils/hash.h.
#define DIFFTEST_STORE_HASH_ROT_DATA 23
#define DIFFTEST_STORE_HASH_ROT_LO 17
#define DIFFTEST_STORE_HASH_ROT_HI 29
#define DIFFTEST_STORE_HASH_MASK_SHIFT 56

typedef struct {
  uint64_t h0, h1, count;
} StoreLogHash;

static StoreLogHash boundary_hash;
static bool hash_enabled = true;
#ifdef CONFIG_STORE_LOG
static StoreLogHash rollback_hash;
#endif

void store_log_hash_reset(void) {
  difftest_hash_init(&boundary_hash.h0, &boundary_hash.h1);
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
  uint64_t value = addr ^ difftest_hash_rotl(data, DIFFTEST_STORE_HASH_ROT_DATA) ^
                   ((uint64_t)mask << DIFFTEST_STORE_HASH_MASK_SHIFT);
  value = difftest_hash_mix(value + boundary_hash.count * DIFFTEST_HASH_COUNT_STEP);
  boundary_hash.h0 = difftest_hash_rotl(boundary_hash.h0 ^ (value + DIFFTEST_STORE_HASH_ADD_LO),
                                      DIFFTEST_STORE_HASH_ROT_LO) * DIFFTEST_HASH_XXH_PRIME1;
  boundary_hash.h1 = difftest_hash_rotl(boundary_hash.h1 + (value ^ DIFFTEST_STORE_HASH_ADD_HI),
                                      DIFFTEST_STORE_HASH_ROT_HI) * DIFFTEST_HASH_XXH_PRIME2;
  boundary_hash.count++;
}

void store_log_hash(uint64_t *lo, uint64_t *hi, uint64_t *count) {
  *lo = boundary_hash.h0;
  *hi = boundary_hash.h1;
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
