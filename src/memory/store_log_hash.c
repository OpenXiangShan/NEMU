/***************************************************************************************
* Copyright (c) 2020-2026 Institute of Computing Technology, Chinese Academy of Sciences
*
* NEMU is licensed under Mulan PSL v2.
* You may obtain a copy of Mulan PSL v2 at:
*          http://license.coscl.org.cn/MulanPSL2
***************************************************************************************/

#include <memory/store_log_hash.h>

#ifdef CONFIG_STORE_LOG_HASH
static DifftestStoreHashState boundary_hash;
static bool hash_enabled = true;
#ifdef CONFIG_STORE_LOG
static DifftestStoreHashState rollback_hash;
#endif

void store_log_hash_reset(void) {
  difftest_store_hash_init(&boundary_hash);
  IFDEF(CONFIG_STORE_LOG, rollback_hash = boundary_hash);
}

void store_log_hash_set_enabled(bool enabled) {
  hash_enabled = enabled;
}

bool store_log_hash_enabled(void) {
  return hash_enabled;
}

void store_log_hash_update(uint64_t addr, uint64_t data, uint8_t mask) {
  if (hash_enabled) difftest_store_hash_update(&boundary_hash, addr, data, mask);
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
